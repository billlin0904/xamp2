//=====================================================================================================================
// Copyright (c) 2018-2025 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/enum.h>
#include <base/uuidof.h>
#include <base/memory.h>
#include <base/pimplptr.h>

#include <stream/stream.h>
#include <stream/eqsettings.h>
#include <stream/iaudioprocessor.h>

XAMP_STREAM_NAMESPACE_BEGIN

class BassParametricEq final : public IAudioProcessor {
    XAMP_DECLARE_MAKE_CLASS_UUID(BassParametricEq, "EBFA0111-594F-4F9D-9131-256451C3BF46")

public:
    BassParametricEq();

    XAMP_PIMPL(BassParametricEq)

    /**
    * A function that starts the audio processing with the given sample rate.
    *
    * @param sample_rate the sample rate at which the audio processing should occur
    *
    * @throws ErrorType if there is an error during the processing
    */
    void Initialize(const AnyMap& config) override;

    void SetEq(const EqSettings& settings);

    /**
    * ReadStream function processes the input samples using bass_util and returns the result.
    *
    * @param samples pointer to the input samples
    * @param num_samples number of samples to process
    * @param out reference to the output BufferRef<float>
    *
    * @return boolean indicating the success of the process
    *
    * @throws None
    */
    bool Process(float const* samples, size_t num_samples, BufferRef<float>& out) override;

    Uuid GetTypeId() const override;

    XAMP_NO_DISCARD std::string_view GetDescription() const noexcept override;

private:    
    class BassParametricEqImpl;
    ScopedPtr<BassParametricEqImpl> impl_;
};

XAMP_STREAM_NAMESPACE_END

