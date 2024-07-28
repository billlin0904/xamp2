//=====================================================================================================================
// Copyright (c) 2018-2024 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <vector>
#include <widget/widget_shared.h>

#include <widget/widget_shared_global.h>
#include <widget/baseservice.h>

class LibrosaInterop {
public:
    LibrosaInterop();

    ~LibrosaInterop();

    bool initial();

    std::pair<std::vector<float>, double> load(const std::string& path,
                                               bool mono=true,
                                               int offset=0,
                                               int duration=0);

    std::vector<std::vector<float>> melspectrogram(const std::vector<float>& y,
                                                   double sr=22050,
                                                   int n_fft=2048,
                                                   int hop_length=512,
                                                   int n_mels=128,
                                                   std::optional<int> win_length = std::nullopt,
                                                   std::optional<double> fmin = std::nullopt,
                                                   std::optional<double> fmax = std::nullopt);

    std::vector<std::vector<float>> power_to_db(const std::vector<std::vector<float>>& S,
                                                double ref=1.0,
                                                double amin=1e-10,
                                                double top_db=80.0);

private:
    class LibrosaInteropImpl;
    AlignPtr<LibrosaInteropImpl> impl_;
};

class LibrosaService : public BaseService {
public:
    explicit LibrosaService(QObject* parent = nullptr);

    QFuture<bool> initialAsync();

    QFuture<bool> loadAsync(const QString &path);
private:
    LibrosaInterop* interop();
    LocalStorage<LibrosaInterop> interop_;
};
