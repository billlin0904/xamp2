#include <widget/baseservice.h>

#undef slots
#include <pybind11/embed.h>
#include <pybind11/stl.h>
#define slots Q_SLOTS

namespace py = pybind11;

class PythonInterop::PythonInteropImpl {
public:
	PythonInteropImpl() = default;
	~PythonInteropImpl() = default;

private:
	py::scoped_interpreter guard_;
	py::gil_scoped_release release_;
};

PythonInterop::PythonInterop()
	: impl_(MakeAlign<PythonInteropImpl>()) {
}

XAMP_PIMPL_IMPL(PythonInterop)
