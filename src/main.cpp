#include "PCH.h"

namespace JM::ElementalArrows
{
    namespace
    {
        struct SoulGemInstance
        {
            RE::TESSoulGem* base{ nullptr };
            RE::ExtraDataList* extraList{ nullptr };
            RE::SOUL_LEVEL soul{ RE::SOUL_LEVEL::kNone };
        };

        std::vector<SoulGemInstance> GetTrappedInstances(
            RE::Actor* a_owner,
            RE::TESSoulGem* a_baseGem)
        {
            std::vector<SoulGemInstance> result;

            if (!a_owner || !a_baseGem) {
                return result;
            }

            auto inventory = a_owner->GetInventory(
                [a_baseGem](RE::TESBoundObject& a_object) {
                    return std::addressof(a_object) == a_baseGem;
                });

            const auto it = inventory.find(a_baseGem);
            if (it == inventory.end()) {
                return result;
            }

            const auto& entry = it->second.second;
            if (!entry || !entry->extraLists) {
                return result;
            }

            for (auto* extraList : *entry->extraLists) {
                if (!extraList) {
                    continue;
                }

                const auto soul = extraList->GetSoulLevel();
                if (soul > RE::SOUL_LEVEL::kNone) {
                    result.push_back({ a_baseGem, extraList, soul });
                }
            }

            return result;
        }

        std::optional<SoulGemInstance> GetBestTrappedInstance(
            RE::Actor* a_owner,
            RE::TESSoulGem* a_baseGem)
        {
            auto instances = GetTrappedInstances(a_owner, a_baseGem);
            if (instances.empty()) {
                return std::nullopt;
            }

            const auto best = std::max_element(
                instances.begin(),
                instances.end(),
                [](const SoulGemInstance& a_lhs, const SoulGemInstance& a_rhs) {
                    return static_cast<std::uint8_t>(a_lhs.soul) <
                           static_cast<std::uint8_t>(a_rhs.soul);
                });

            return *best;
        }
    }

    namespace Papyrus
    {
        constexpr auto kScriptName = "JM_EA_Native";

        bool IsAvailable(RE::StaticFunctionTag*)
        {
            return true;
        }

        std::int32_t CountSoulTrappedGems(
            RE::StaticFunctionTag*,
            RE::Actor* a_owner,
            RE::TESSoulGem* a_baseGem)
        {
            return static_cast<std::int32_t>(
                GetTrappedInstances(a_owner, a_baseGem).size());
        }

        std::int32_t GetBestTrappedSoulLevel(
            RE::StaticFunctionTag*,
            RE::Actor* a_owner,
            RE::TESSoulGem* a_baseGem)
        {
            const auto best = GetBestTrappedInstance(a_owner, a_baseGem);
            return best ?
                static_cast<std::int32_t>(best->soul) :
                static_cast<std::int32_t>(RE::SOUL_LEVEL::kNone);
        }

        std::int32_t ConsumeBestSoulTrappedGem(
            RE::StaticFunctionTag*,
            RE::Actor* a_owner,
            RE::TESSoulGem* a_baseGem)
        {
            const auto best = GetBestTrappedInstance(a_owner, a_baseGem);
            if (!best || !best->extraList || !best->base) {
                return static_cast<std::int32_t>(RE::SOUL_LEVEL::kNone);
            }

            const auto soul = best->soul;

            a_owner->RemoveItem(
                best->base,
                1,
                RE::ITEM_REMOVE_REASON::kRemove,
                best->extraList,
                nullptr);

            logger::info(
                "Consumed soul-trapped gem {:08X}; trapped soul level {}",
                best->base->GetFormID(),
                static_cast<std::int32_t>(soul));

            return static_cast<std::int32_t>(soul);
        }

        bool Register(RE::BSScript::IVirtualMachine* a_vm)
        {
            if (!a_vm) {
                return false;
            }

            a_vm->RegisterFunction("IsAvailable", kScriptName, IsAvailable);
            a_vm->RegisterFunction(
                "CountSoulTrappedGems",
                kScriptName,
                CountSoulTrappedGems);
            a_vm->RegisterFunction(
                "GetBestTrappedSoulLevel",
                kScriptName,
                GetBestTrappedSoulLevel);
            a_vm->RegisterFunction(
                "ConsumeBestSoulTrappedGem",
                kScriptName,
                ConsumeBestSoulTrappedGem);

            logger::info("Registered Papyrus class {}", kScriptName);
            return true;
        }
    }

    void InitializeLogging()
    {
        if (const auto logDir = SKSE::log::log_directory()) {
            auto path = *logDir / "JM_ElementalArrows.log";
            auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(
                path.string(), true);
            auto log = std::make_shared<spdlog::logger>(
                "global log", std::move(sink));
            spdlog::set_default_logger(std::move(log));
            spdlog::set_level(spdlog::level::info);
            spdlog::flush_on(spdlog::level::info);
        }
    }
}

SKSEPluginLoad(const SKSE::LoadInterface* a_skse)
{
    SKSE::Init(a_skse);
    JM::ElementalArrows::InitializeLogging();

    logger::info("JM_ElementalArrows native support loading");

    const auto runtime = REL::Module::get().version();
    if (runtime != SKSE::RUNTIME_SSE_1_5_97) {
        logger::critical(
            "Unsupported Skyrim runtime {}. JM_ElementalArrows intentionally targets 1.5.97.",
            runtime.string());
        return false;
    }

    const auto* papyrus = SKSE::GetPapyrusInterface();
    if (!papyrus) {
        logger::critical("SKSE Papyrus interface unavailable");
        return false;
    }

    if (!papyrus->Register(JM::ElementalArrows::Papyrus::Register)) {
        logger::critical("Failed to register JM_EA_Native");
        return false;
    }

    logger::info("JM_ElementalArrows native support loaded successfully");
    return true;
}
