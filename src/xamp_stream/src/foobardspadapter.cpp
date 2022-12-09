#include <map>
#include <base/dll.h>
#include <base/logger_impl.h>
#include <base/exception.h>
#include <base/fs.h>
#include <stream/foobardspadapter.h>

#if 0
#include <combaseapi.h>
#include <shellapi.h>

#include <shared/shared.h>
#include <SDK/component.h>
#include <SDK/foobar2000-dsp.h>

namespace std {
	template<> struct hash<GUID> {
		size_t operator()(const GUID& guid) const noexcept {
			const auto* guid_int = static_cast<int*>((void*)&guid);
			return guid_int[0] ^ guid_int[1] ^ guid_int[2] ^ guid_int[3];
		}
	};
}

static std::ostream& operator<<(std::ostream& os, REFGUID guid) {
	os << std::uppercase;
	os.width(8);
	os << std::hex << guid.Data1 << '-';

	os.width(4);
	os << std::hex << guid.Data2 << '-';

	os.width(4);
	os << std::hex << guid.Data3 << '-';

	os.width(2);
	os << std::hex
		<< static_cast<short>(guid.Data4[0])
		<< static_cast<short>(guid.Data4[1])
		<< '-'
		<< static_cast<short>(guid.Data4[2])
		<< static_cast<short>(guid.Data4[3])
		<< static_cast<short>(guid.Data4[4])
		<< static_cast<short>(guid.Data4[5])
		<< static_cast<short>(guid.Data4[6])
		<< static_cast<short>(guid.Data4[7]);
	os << std::nouppercase;
	return os;
}

namespace xamp::stream {

XAMP_DECLARE_LOG_NAME(FoobarDSPAdapter);

struct FoobarDspContext {
	std::string name;
	GUID guid{ 0 };
	service_ptr_t<service_base> svc;
	service_ptr_t<service_base> dsp_entry_svc;
	service_ptr resampler_entry_svc;
	service_ptr_t<dsp> dsp;
	dsp_preset_impl preset;
	dsp_entry* dsp_entry{ nullptr };	
};

class Foobar2000Runtime final : public foobar2000_api {
public:
	Foobar2000Runtime() = default;

	virtual ~Foobar2000Runtime() = default;

	service_class_ref service_enum_find_class(const GUID& p_guid) override {
		return nullptr;
	}

	bool service_enum_create(service_ptr_t<service_base>& p_out, service_class_ref p_class, t_size p_index) override {
		if (p_index >= svcs_.size()) {
			return false;
		}
		p_out = svcs_[p_index];
		return true;
	}

	t_size service_enum_get_count(service_class_ref p_class) override {
		if (!p_class) {
			return 0;
		}
		return svcs_.size();
	}

	HWND get_main_window() override {
		return ::GetActiveWindow();
	}

	bool assert_main_thread() override {
		return true;
	}

	bool is_main_thread() override {
		return true;
	}

	bool is_shutting_down() override {
		return is_shutdown_;
	}

	const char* get_profile_path() override {
		return "";
	}

	bool is_initializing() override {
		return is_init_;
	}

	bool is_portable_mode_enabled() override {
		return true;
	}

	bool is_quiet_mode_enabled() override {
		return false;
	}

	void add_svc(service_ptr_t<service_base> svc) {
		svcs_.push_back(svc);
	}

	void remove_svc(service_ptr_t<service_base> svc, dsp_chain_config &config) {
		int32_t index = 0;
		auto itr = std::find_if(svcs_.begin(), 
			svcs_.end(), 
			[svc, &index](auto exist_svc) {
			auto found = exist_svc.get_ptr() == svc.get_ptr();
			if (found) {
				++index;
			}
			return found;
			});
		if (itr != svcs_.end()) {
			svcs_.erase(itr);
			config.remove_item(index);
		}
	}

	bool is_init_{ false };
	bool is_shutdown_{ false };
	std::vector<service_ptr_t<service_base>> svcs_;
};

class FoobarDspAdapter::FoobarDspAdapterImpl {
public:
	FoobarDspAdapterImpl() {
		logger_ = LoggerManager::GetInstance().GetLogger(kFoobarDSPAdapterLoggerName);
		LoadDllAndInitFoobarClient("xamp_stream.dll");
		LoadDllAndInitFoobarClient("foo_dsp_std.dll");
		EnumFoobarDSP();
	}

	~FoobarDspAdapterImpl() = default;

	void LoadDllAndInitFoobarClient(std::string const &file_name) {
		try {
			dll_ = LoadModule(file_name);
		} catch (const std::exception &ex) {
			XAMP_LOG_E(logger_, ex.what());
			return;
		}

		auto app = ::GetModuleHandleA(nullptr);
		DllFunction<foobar2000_client* (foobar2000_api*, HINSTANCE)> foobar2000_get_interface(dll_, "foobar2000_get_interface");
		foobar_client_ = foobar2000_get_interface(&foobar_runtime_, app);
		foobar_client_->services_init(true);
	}

	void EnumFoobarDSP() {
		const auto svc_list = foobar_client_->get_service_list();
		auto next_svc_entry = svc_list;

		for (; next_svc_entry != nullptr; next_svc_entry = next_svc_entry->__internal__next) {
			service_ptr_t<service_base> svc;
			next_svc_entry->instance_create(svc);
			if (!svc.is_valid()) {
				continue;
			}

			service_ptr dsp_entry_svc;
			if (!svc->service_query(dsp_entry_svc, dsp_entry::class_guid)) {
				continue;
			}

			service_ptr resampler_entry_svc;
			dsp_entry_svc->service_query(resampler_entry_svc, resampler_entry::class_guid);

			auto* dsp_entry_ptr = static_cast<dsp_entry*>(dsp_entry_svc.get_ptr());
			pfc::string name;
			dsp_entry_ptr->get_name(name);
			XAMP_LOG_D(logger_, "Load Foobar2000 DSP name: {} guid: {}", name.c_str(), dsp_entry_ptr->get_guid());
			const auto foobar_dsp = std::make_shared<FoobarDspContext>();
			dsp_entry_ptr->get_default_preset(foobar_dsp->preset);
			foobar_dsp->svc = svc;
			foobar_dsp->dsp_entry = dsp_entry_ptr;
			foobar_dsp->guid = dsp_entry_ptr->get_guid();
			foobar_dsp->dsp_entry_svc = dsp_entry_svc;
			foobar_dsp->resampler_entry_svc = resampler_entry_svc;
			foobar_dsp->name = name.c_str();
			dsp_entry_ptr->instantiate(foobar_dsp->dsp, foobar_dsp->preset);
			foobar_dsp_[name.c_str()] = foobar_dsp;			
		}
	}

	std::vector<std::string> GetAvailableDSPs() const {
		return Keys(foobar_dsp_);
	}

	void ConfigPopup(const std::string& name, uint64_t hwnd) {
		if (!foobar_dsp_.contains(name)) {
			return;
		}
		auto& dsp = foobar_dsp_[name];
		if (dsp->dsp_entry->have_config_popup()) {
			dsp->dsp_entry->show_config_popup(dsp->preset, reinterpret_cast<HWND>(hwnd));
		}
	}

	void AddDSPChain(const std::string& name) {
		if (!foobar_dsp_.contains(name)) {
			return;
		}
		auto& dsp = foobar_dsp_[name];
		dsp_chain_config_.add_item(dsp->preset);
		foobar_runtime_.add_svc(dsp->dsp_entry_svc);
	}

	void RemoveDSPChain(const std::string& name) {
		if (!foobar_dsp_.contains(name)) {
			return;
		}
		auto& dsp = foobar_dsp_[name];
		dsp->dsp_entry->get_default_preset(dsp->preset);
		foobar_runtime_.remove_svc(dsp->dsp_entry_svc, dsp_chain_config_);
	}

	void Start(uint32_t output_sample_rate) {
		output_sample_rate_ = output_sample_rate;
	}

	void Init(uint32_t input_sample_rate) {
		input_sample_rate_ = input_sample_rate;
		
		for (const auto &context : Values(foobar_dsp_)) {
			if (!context->resampler_entry_svc.is_valid()) {
				continue;
			}
			auto* resampler_entry_ptr = static_cast<resampler_entry*>(context->resampler_entry_svc.get_ptr());
			resampler_entry_ptr->is_conversion_supported(input_sample_rate_, output_sample_rate_);
		}
	}

	bool Process(float const* samples, uint32_t num_samples, BufferRef<float>& output) {
		input_data_.resize(num_samples);

		for (auto i = 0; i < num_samples; ++i) {
			input_data_[i] = static_cast<double>(samples[i]);
		}

		dsp_chunk_list_impl chunk_list;
		audio_chunk_impl chunk(input_data_.data(), input_data_.size(), 2, input_sample_rate_);

		chunk_list.add_chunk(&chunk);
		abort_callback_impl callback;
		dsp_manager_.run(&chunk_list, nullptr, dsp::FLUSH, callback);

		output.maybe_resize(num_samples);

		for (auto i = 0; i < num_samples; ++i) {
			output.data()[i] = static_cast<float>(input_data_[i]);
		}

		return true;
	}

	void Flush() {
		dsp_manager_.flush();
	}

	bool IsActive() const {
		return dsp_manager_.is_active();
	}

	void CloneDSPChainConfig(const FoobarDspAdapter& adapter) {
		dsp_chain_config_ = adapter.impl_->dsp_chain_config_;
		dsp_manager_.set_config(dsp_chain_config_);
		foobar_runtime_.svcs_ = adapter.impl_->foobar_runtime_.svcs_;
		adapter.impl_->foobar_runtime_.svcs_.clear();
	}

private:
	uint32_t input_sample_rate_{ 0 };
	uint32_t output_sample_rate_{ 0 };
	ModuleHandle dll_;
	foobar2000_client* foobar_client_{ nullptr };
	Foobar2000Runtime foobar_runtime_;
	dsp_manager dsp_manager_;
	dsp_chain_config_impl dsp_chain_config_;
	std::map<std::string, std::shared_ptr<FoobarDspContext>> foobar_dsp_;
	Vector<double> input_data_;
	std::shared_ptr<Logger> logger_;
};

FoobarDspAdapter::FoobarDspAdapter()
	: impl_(MakeAlign<FoobarDspAdapterImpl>()) {
}

XAMP_PIMPL_IMPL(FoobarDspAdapter)

void FoobarDspAdapter::Init(uint32_t input_sample_rate) {
	return impl_->Init(input_sample_rate);
}

std::vector<std::string> FoobarDspAdapter::GetAvailableDSPs() const {
	return impl_->GetAvailableDSPs();
}

void FoobarDspAdapter::ConfigPopup(const std::string& name, uint64_t hwnd) {
	impl_->ConfigPopup(name, hwnd);
}

void FoobarDspAdapter::Start(uint32_t output_sample_rate) {
	impl_->Start(output_sample_rate);
}

bool FoobarDspAdapter::Process(float const* samples, uint32_t num_samples, BufferRef<float>& output) {
	return impl_->Process(samples, num_samples, output);
}

Uuid FoobarDspAdapter::GetTypeId() const {
	return Id;
}

std::string_view FoobarDspAdapter::GetDescription() const noexcept {
	return "Foobar2000 Dsp Adapter";
}

void FoobarDspAdapter::Flush() {
	return impl_->Flush();
}

void FoobarDspAdapter::AddDSPChain(const std::string& name) {
	impl_->AddDSPChain(name);
}

void FoobarDspAdapter::RemoveDSPChain(const std::string& name) {
	impl_->RemoveDSPChain(name);
}

void FoobarDspAdapter::CloneDSPChainConfig(const FoobarDspAdapter& adapter) {
	impl_->CloneDSPChainConfig(adapter);
}

bool FoobarDspAdapter::IsActive() const {
	return impl_->IsActive();
}
#else

namespace xamp::stream {

class FoobarDspAdapter::FoobarDspAdapterImpl {
public:
    FoobarDspAdapterImpl() = default;
    ~FoobarDspAdapterImpl() = default;
};

FoobarDspAdapter::FoobarDspAdapter() {
}

XAMP_PIMPL_IMPL(FoobarDspAdapter)

void FoobarDspAdapter::Init(const DspConfig& config) {
	
}

std::vector<std::string> FoobarDspAdapter::GetAvailableDSPs() const {
    return {};
}

void FoobarDspAdapter::ConfigPopup(const std::string& name, uint64_t hwnd) {
	
}

void FoobarDspAdapter::Start(const DspConfig& config) {

}

bool FoobarDspAdapter::Process(float const* samples, uint32_t num_samples, BufferRef<float>& output) {
	return true;
}

uint32_t FoobarDspAdapter::Process(float const* samples, float* out, uint32_t num_samples) {
	return 0;
}

Uuid FoobarDspAdapter::GetTypeId() const {
	return Id;
}

std::string_view FoobarDspAdapter::GetDescription() const noexcept {
	return "Foobar2000 Dsp Adapter";
}

void FoobarDspAdapter::Flush() {
}

void FoobarDspAdapter::AddDSPChain(const std::string& name) {
}

void FoobarDspAdapter::RemoveDSPChain(const std::string& name) {
}

void FoobarDspAdapter::CloneDSPChainConfig(const FoobarDspAdapter& adapter) {
}

bool FoobarDspAdapter::IsActive() const {
	return false;
}
#endif

}
