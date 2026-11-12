/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <map>
#include <mutex>

#include "ohos.update.proj.hpp"
#include "ohos.update.impl.hpp"
#include "taihe/runtime.hpp"

#include "ani_common_conveter.h"
#include "ani_local_updater.h"
#include "ani_restorer.h"
#include "ani_updater.h"

using namespace OHOS::UpdateService;
namespace {
class UpdaterImpl {
public:
    explicit UpdaterImpl(const UpgradeInfo &upgradeInfo) : upgradeInfo_(upgradeInfo) {}

    ohos::update::CheckResult CheckNewVersionSync()
    {
        ENGINE_LOGI("%{public}s method invoked", __func__);
        return GetAniUpdater()->CheckNewVersionSync();
    }

    ohos::update::NewVersionInfo GetNewVersionInfoSync()
    {
        ENGINE_LOGI("%{public}s method invoked", __func__);
        return GetAniUpdater()->GetNewVersionInfoSync();
    }

    taihe::array<ohos::update::ComponentDescription> GetNewVersionDescriptionSync(
        const ohos::update::VersionDigestInfo &versionDigestInfo,
        const ohos::update::DescriptionOptions &descriptionOptions)
    {
        ENGINE_LOGI("%{public}s method invoked", __func__);
        return GetAniUpdater()->GetNewVersionDescriptionSync(versionDigestInfo, descriptionOptions);
    }

    ohos::update::CurrentVersionInfo GetCurrentVersionInfoSync()
    {
        ENGINE_LOGI("%{public}s method invoked", __func__);
        return GetAniUpdater()->GetCurrentVersionInfoSync();
    }

    taihe::array<ohos::update::ComponentDescription> GetCurrentVersionDescriptionSync(
        ohos::update::DescriptionOptions const & descriptionOptions)
    {
        ENGINE_LOGI("%{public}s method invoked", __func__);
        return GetAniUpdater()->GetCurrentVersionDescriptionSync(descriptionOptions);
    }

    ohos::update::TaskInfo GetTaskInfoSync()
    {
        ENGINE_LOGI("%{public}s method invoked", __func__);
        return GetAniUpdater()->GetTaskInfoSync();
    }

    void DownloadSync(const ohos::update::VersionDigestInfo &versionDigestInfo,
        const ohos::update::DownloadOptions &downloadOptions)
    {
        ENGINE_LOGI("%{public}s method invoked", __func__);
        GetAniUpdater()->DownloadSync(versionDigestInfo, downloadOptions);
    }

    void ResumeDownloadSync(const ohos::update::VersionDigestInfo &versionDigestInfo,
        const ohos::update::ResumeDownloadOptions &resumeDownloadOptions)
    {
        ENGINE_LOGI("%{public}s method invoked", __func__);
        GetAniUpdater()->ResumeDownloadSync(versionDigestInfo, resumeDownloadOptions);
    }

    void PauseDownloadSync(const ohos::update::VersionDigestInfo &versionDigestInfo,
        const ohos::update::PauseDownloadOptions &pauseDownloadOptions)
    {
        ENGINE_LOGI("%{public}s method invoked", __func__);
        GetAniUpdater()->PauseDownloadSync(versionDigestInfo, pauseDownloadOptions);
    }

    void UpgradeSync(const ohos::update::VersionDigestInfo &versionDigestInfo,
        const ohos::update::UpgradeOptions &upgradeOptions)
    {
        ENGINE_LOGI("%{public}s method invoked", __func__);
        GetAniUpdater()->UpgradeSync(versionDigestInfo, upgradeOptions);
    }

    void ClearErrorSync(const ohos::update::VersionDigestInfo &versionDigestInfo,
        const ohos::update::ClearOptions &clearOptions)
    {
        ENGINE_LOGI("%{public}s method invoked", __func__);
        GetAniUpdater()->ClearErrorSync(versionDigestInfo, clearOptions);
    }

    ohos::update::UpgradePolicy GetUpgradePolicySync()
    {
        ENGINE_LOGI("%{public}s method invoked", __func__);
        return GetAniUpdater()->GetUpgradePolicySync();
    }

    void SetUpgradePolicySync(const ohos::update::UpgradePolicy &policy)
    {
        ENGINE_LOGI("%{public}s method invoked", __func__);
        GetAniUpdater()->SetUpgradePolicySync(policy);
    }

    void TerminateUpgradeSync()
    {
        ENGINE_LOGI("%{public}s method invoked", __func__);
        GetAniUpdater()->TerminateUpgradeSync();
    }

    void On(const ohos::update::EventClassifyInfo &eventClassifyInfo,
        taihe::callback_view<void(const ohos::update::EventInfo &)> taskCallback)
    {
        ENGINE_LOGI("%{public}s method invoked", __func__);
        GetAniUpdater()->On(taskCallback);
    }

    void Off(const ::ohos::update::EventClassifyInfo &eventClassifyInfo,
        taihe::optional_view<taihe::callback<void(const ohos::update::EventInfo &)>> taskCallback)
    {
        ENGINE_LOGI("%{public}s method invoked", __func__);
        if (taskCallback) {
            GetAniUpdater()->Off(*taskCallback);
            return;
        }
        GetAniUpdater()->OffAll();
    }

private:
    static inline std::mutex aniUpdatersMapMutex_;
    static inline std::map<UpgradeInfo, std::shared_ptr<AniUpdater>> aniUpdatersMap_;
    UpgradeInfo upgradeInfo_;
    std::shared_ptr<AniUpdater> GetAniUpdater()
    {
        std::lock_guard<std::mutex> lock(aniUpdatersMapMutex_);
        auto iter = aniUpdatersMap_.find(upgradeInfo_);
        if (iter != aniUpdatersMap_.end()) {
            return iter->second;
        }
        auto aniUpdater = std::make_shared<AniUpdater>(upgradeInfo_);
        aniUpdatersMap_.emplace(upgradeInfo_, aniUpdater);
        return aniUpdater;
    }
};

class RestorerImpl {
public:
    RestorerImpl() = default;

    void FactoryResetSync()
    {
        ENGINE_LOGI("%{public}s method invoked", __func__);
        GetAniRestorer()->FactoryResetSync();
    }

    void ForceFactoryResetSync()
    {
        ENGINE_LOGI("%{public}s method invoked", __func__);
        GetAniRestorer()->ForceFactoryResetSync();
    }

    void DeepFactoryResetSync(const ohos::update::FactoryResetStrategy &factoryResetStrategy)
    {
        ENGINE_LOGI("%{public}s method invoked", __func__);
        GetAniRestorer()->DeepFactoryResetSync(factoryResetStrategy);
    }

    ohos::update::FactoryResetInfo GetDeepFactoryResetInfoSync(
        const ohos::update::FactoryResetStrategy &factoryResetStrategy)
    {
        ENGINE_LOGI("%{public}s method invoked", __func__);
        return GetAniRestorer()->GetDeepFactoryResetInfoSync(factoryResetStrategy);
    }

private:
    static inline std::shared_ptr<AniRestorer> aniRestorer_ = nullptr;
    static inline std::once_flag aniRestorerFlag_;
    std::shared_ptr<AniRestorer> GetAniRestorer()
    {
        std::call_once(aniRestorerFlag_, []() {
            aniRestorer_ = std::make_shared<AniRestorer>();
        });
        return aniRestorer_;
    }
};

class LocalUpdaterImpl {
public:
    LocalUpdaterImpl() = default;

    void VerifyUpgradePackageSync(const ohos::update::UpgradeFile &upgradeFile, taihe::string_view certsFile)
    {
        ENGINE_LOGI("%{public}s method invoked", __func__);
        GetLocalUpdater()->VerifyUpgradePackageSync(upgradeFile, certsFile);
    }

    void ApplyNewVersionSync(taihe::array_view<ohos::update::UpgradeFile> upgradeFiles)
    {
        ENGINE_LOGI("%{public}s method invoked", __func__);
        GetLocalUpdater()->ApplyNewVersionSync(upgradeFiles);
    }

    void On(const ohos::update::EventClassifyInfo &eventClassifyInfo,
        taihe::callback_view<void(const ohos::update::EventInfo &)> taskCallback)
    {
        ENGINE_LOGI("%{public}s method invoked", __func__);
        GetLocalUpdater()->On(taskCallback);
    }

    void Off(const ::ohos::update::EventClassifyInfo &eventClassifyInfo,
        taihe::optional_view<taihe::callback<void(const ohos::update::EventInfo &)>> taskCallback)
    {
        ENGINE_LOGI("%{public}s method invoked", __func__);
        if (taskCallback) {
            GetLocalUpdater()->Off(*taskCallback);
            return;
        }
        GetLocalUpdater()->OffAll();
    }

private:
    static inline std::shared_ptr<AniLocalUpdater> localUpdater_ = nullptr;
    static inline std::once_flag aniLocalUpdaterFlag_;
    std::shared_ptr<AniLocalUpdater> GetLocalUpdater()
    {
        std::call_once(aniLocalUpdaterFlag_, []() {
            localUpdater_ = std::make_shared<AniLocalUpdater>();
        });
        return localUpdater_;
    }
};

ohos::update::Updater GetOnlineUpdater(const ohos::update::UpgradeInfo &upgradeInfo)
{
    const auto &info = AniCommonConverter::Converter(upgradeInfo);
    return taihe::make_holder<UpdaterImpl, ohos::update::Updater>(info);
}

ohos::update::Restorer GetRestorer()
{
    return taihe::make_holder<RestorerImpl, ohos::update::Restorer>();
}

ohos::update::LocalUpdater GetLocalUpdater()
{
    return taihe::make_holder<LocalUpdaterImpl, ohos::update::LocalUpdater>();
}
} // namespace

// Since these macros are auto-generate, lint will cause false positive.
// NOLINTBEGIN
TH_EXPORT_CPP_API_GetOnlineUpdater(GetOnlineUpdater);
TH_EXPORT_CPP_API_GetRestorer(GetRestorer);
TH_EXPORT_CPP_API_GetLocalUpdater(GetLocalUpdater);
// NOLINTEND
