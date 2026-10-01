#include "PCH.h"

#include <array>
#include <string_view>

namespace JM::ElementalArrows
{
    namespace
    {
        constexpr std::string_view kElementalArrowsPlugin = "ElementalArrows.esp";
        constexpr std::string_view kSkyrimPlugin = "Skyrim.esm";
        constexpr RE::FormID kGemArrowLocalFormID = 0x000213E9;

        enum class GemFamily : std::uint8_t
        {
            kPetty = 0,
            kLesser,
            kCommon,
            kGreater,
            kGrand,
            kBlack,
            kCount
        };

        struct GemFormSpec
        {
            RE::FormID localFormID;
            GemFamily family;
            std::string_view name;
            RE::TESSoulGem* form{ nullptr };
        };

        constexpr std::array<std::int32_t, 6> kEmptyYield{ 3, 5, 10, 20, 30, 30 };
        constexpr std::array<std::int32_t, 6> kSoulYield{ 0, 10, 20, 40, 75, 100 };
        constexpr std::size_t kGemFormCount = 12;

        std::array<GemFormSpec, kGemFormCount> g_gemForms{
            GemFormSpec{ 0x0002E4E2, GemFamily::kPetty, "Petty Soul Gem" },
            GemFormSpec{ 0x0002E4E4, GemFamily::kLesser, "Lesser Soul Gem" },
            GemFormSpec{ 0x0002E4E6, GemFamily::kCommon, "Common Soul Gem" },
            GemFormSpec{ 0x0002E4F4, GemFamily::kGreater, "Greater Soul Gem" },
            GemFormSpec{ 0x0002E4FC, GemFamily::kGrand, "Grand Soul Gem" },
            GemFormSpec{ 0x0002E500, GemFamily::kBlack, "Black Soul Gem" },
            GemFormSpec{ 0x0002E4E3, GemFamily::kPetty, "Petty Soul Gem (filled base)" },
            GemFormSpec{ 0x0002E4E5, GemFamily::kLesser, "Lesser Soul Gem (filled base)" },
            GemFormSpec{ 0x0002E4F3, GemFamily::kCommon, "Common Soul Gem (filled base)" },
            GemFormSpec{ 0x0002E4FB, GemFamily::kGreater, "Greater Soul Gem (filled base)" },
            GemFormSpec{ 0x0002E4FF, GemFamily::kGrand, "Grand Soul Gem (filled base)" },
            GemFormSpec{ 0x0002E504, GemFamily::kBlack, "Black Soul Gem (filled base)" }
        };

        struct InventorySnapshot
        {
            std::array<std::array<std::int32_t, 6>, kGemFormCount> soulCounts{};
            std::int32_t gemArrowCount{ 0 };
            bool valid{ false };
        };

        RE::TESAmmo* g_gemArrow{ nullptr };
        InventorySnapshot g_snapshot;

        constexpr std::size_t SoulIndex(RE::SOUL_LEVEL a_soul)
        {
            const auto value = static_cast<std::int32_t>(a_soul);
            return static_cast<std::size_t>(std::clamp(value, 0, 5));
        }

        std::int32_t GetYield(GemFamily a_family, RE::SOUL_LEVEL a_soul)
        {
            const auto familyIndex = static_cast<std::size_t>(a_family);
            const auto baseYield = kEmptyYield[familyIndex];

            if (a_soul == RE::SOUL_LEVEL::kNone) {
                return baseYield;
            }

            if (a_family == GemFamily::kBlack && a_soul == RE::SOUL_LEVEL::kGrand) {
                return 120;
            }

            const auto soulIndex = SoulIndex(a_soul);
            return (std::max)(baseYield, kSoulYield[soulIndex]);
        }

        std::int32_t CountObject(RE::PlayerCharacter* a_player, RE::TESBoundObject* a_object)
        {
            if (!a_player || !a_object) {
                return 0;
            }

            const auto inventory = a_player->GetInventory(
                [a_object](RE::TESBoundObject& a_candidate) {
                    return std::addressof(a_candidate) == a_object;
                });

            const auto it = inventory.find(a_object);
            return it != inventory.end() ? (std::max)(0, it->second.first) : 0;
        }

        InventorySnapshot CaptureSnapshot()
        {
            InventorySnapshot snapshot;
            auto* player = RE::PlayerCharacter::GetSingleton();
            if (!player || !g_gemArrow) {
                return snapshot;
            }

            for (std::size_t i = 0; i < g_gemForms.size(); ++i) {
                const auto& spec = g_gemForms[i];
                if (!spec.form) {
                    continue;
                }

                auto inventory = player->GetInventory(
                    [form = spec.form](RE::TESBoundObject& a_candidate) {
                        return std::addressof(a_candidate) == form;
                    });

                const auto it = inventory.find(spec.form);
                if (it == inventory.end()) {
                    continue;
                }

                const auto totalCount = (std::max)(0, it->second.first);
                auto* entry = it->second.second.get();
                std::int32_t representedCount = 0;
                const auto baseSoul = spec.form->GetContainedSoul();

                if (entry && entry->extraLists) {
                    for (auto* extraList : *entry->extraLists) {
                        if (!extraList) {
                            continue;
                        }

                        const auto stackCount = (std::max)(1, extraList->GetCount());
                        auto soul = extraList->GetSoulLevel();
                        if (soul == RE::SOUL_LEVEL::kNone) {
                            soul = baseSoul;
                        }

                        snapshot.soulCounts[i][SoulIndex(soul)] += stackCount;
                        representedCount += stackCount;
                    }
                }

                const auto genericCount = (std::max)(0, totalCount - representedCount);
                if (genericCount > 0) {
                    snapshot.soulCounts[i][SoulIndex(baseSoul)] += genericCount;
                }
            }

            snapshot.gemArrowCount = CountObject(player, g_gemArrow);
            snapshot.valid = true;
            return snapshot;
        }

        void RefreshSnapshot()
        {
            g_snapshot = CaptureSnapshot();
        }

        void ReconcileGemArrowCraft()
        {
            auto current = CaptureSnapshot();
            if (!current.valid) {
                logger::warn("Unable to capture post-craft soul-gem snapshot");
                g_snapshot = current;
                return;
            }

            if (!g_snapshot.valid) {
                logger::warn("No pre-craft snapshot available; skipping bonus calculation");
                g_snapshot = current;
                return;
            }

            std::int32_t expectedYield = 0;
            std::int32_t consumedGemCount = 0;

            for (std::size_t i = 0; i < g_gemForms.size(); ++i) {
                for (std::size_t soulIndex = 0; soulIndex < 6; ++soulIndex) {
                    const auto before = g_snapshot.soulCounts[i][soulIndex];
                    const auto after = current.soulCounts[i][soulIndex];
                    const auto removed = (std::max)(0, before - after);
                    if (removed <= 0) {
                        continue;
                    }

                    const auto soul = static_cast<RE::SOUL_LEVEL>(soulIndex);
                    const auto perGemYield = GetYield(g_gemForms[i].family, soul);
                    expectedYield += removed * perGemYield;
                    consumedGemCount += removed;

                    logger::info(
                        "Craft consumed {} x {} with soul level {}; expected contribution {} Gem Arrows",
                        removed,
                        g_gemForms[i].name,
                        static_cast<std::int32_t>(soul),
                        removed * perGemYield);
                }
            }

            const auto actualCrafted = (std::max)(0, current.gemArrowCount - g_snapshot.gemArrowCount);
            const auto bonus = (std::max)(0, expectedYield - actualCrafted);

            if (consumedGemCount <= 0) {
                logger::warn("Gem Arrow craft detected but no supported soul-gem consumption was observed");
                g_snapshot = current;
                return;
            }

            if (bonus > 0) {
                auto* player = RE::PlayerCharacter::GetSingleton();
                if (player) {
                    player->AddObjectToContainer(g_gemArrow, nullptr, bonus, nullptr);
                    current.gemArrowCount += bonus;
                }
            }

            logger::info(
                "Gem Arrow craft reconciled: consumedGems={}, vanillaOutput={}, expectedOutput={}, bonusAdded={}",
                consumedGemCount,
                actualCrafted,
                expectedYield,
                bonus);

            g_snapshot = current;
        }

        class EventHandler final :
            public RE::BSTEventSink<RE::ItemCrafted::Event>,
            public RE::BSTEventSink<RE::MenuOpenCloseEvent>
        {
        public:
            static EventHandler* GetSingleton()
            {
                static EventHandler singleton;
                return std::addressof(singleton);
            }

            RE::BSEventNotifyControl ProcessEvent(
                const RE::ItemCrafted::Event* a_event,
                RE::BSTEventSource<RE::ItemCrafted::Event>*) override
            {
                if (!a_event) {
                    return RE::BSEventNotifyControl::kContinue;
                }

                const bool isGemArrowCraft = a_event->item == g_gemArrow;
                if (const auto* tasks = SKSE::GetTaskInterface()) {
                    tasks->AddTask([isGemArrowCraft]() {
                        if (isGemArrowCraft) {
                            ReconcileGemArrowCraft();
                        } else {
                            RefreshSnapshot();
                        }
                    });
                }

                return RE::BSEventNotifyControl::kContinue;
            }

            RE::BSEventNotifyControl ProcessEvent(
                const RE::MenuOpenCloseEvent* a_event,
                RE::BSTEventSource<RE::MenuOpenCloseEvent>*) override
            {
                if (!a_event || a_event->menuName != RE::CraftingMenu::MENU_NAME) {
                    return RE::BSEventNotifyControl::kContinue;
                }

                if (a_event->opening) {
                    if (const auto* tasks = SKSE::GetTaskInterface()) {
                        tasks->AddTask([]() { RefreshSnapshot(); });
                    }
                } else {
                    g_snapshot = {};
                }

                return RE::BSEventNotifyControl::kContinue;
            }
        };

        bool ResolveForms()
        {
            auto* dataHandler = RE::TESDataHandler::GetSingleton();
            if (!dataHandler) {
                logger::critical("TESDataHandler unavailable");
                return false;
            }

            g_gemArrow = dataHandler->LookupForm<RE::TESAmmo>(kGemArrowLocalFormID, kElementalArrowsPlugin);
            if (!g_gemArrow) {
                logger::critical("FumaMagicArrowGem not found in {}", kElementalArrowsPlugin);
                return false;
            }

            for (auto& spec : g_gemForms) {
                spec.form = dataHandler->LookupForm<RE::TESSoulGem>(spec.localFormID, kSkyrimPlugin);
                if (!spec.form) {
                    logger::critical("Failed to resolve {} ({:08X})", spec.name, spec.localFormID);
                    return false;
                }
            }

            return true;
        }

        bool RegisterEventSinks()
        {
            auto* handler = EventHandler::GetSingleton();

            auto* craftedSource = RE::ItemCrafted::GetEventSource();
            if (!craftedSource) {
                logger::critical("ItemCrafted event source unavailable");
                return false;
            }
            craftedSource->AddEventSink(handler);

            auto* ui = RE::UI::GetSingleton();
            if (!ui) {
                logger::critical("UI singleton unavailable");
                return false;
            }
            ui->AddEventSink<RE::MenuOpenCloseEvent>(handler);

            logger::info("Registered ItemCrafted and Crafting Menu event sinks");
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

    bool Initialize()
    {
        if (!ResolveForms()) {
            return false;
        }

        if (!RegisterEventSinks()) {
            return false;
        }

        logger::info("JM_ElementalArrows crafting integration initialized");
        return true;
    }
}

SKSEPluginLoad(const SKSE::LoadInterface* a_skse)
{
    SKSE::Init(a_skse);
    JM::ElementalArrows::InitializeLogging();

    logger::info("JM_ElementalArrows loading");

    const auto runtime = REL::Module::get().version();
    if (runtime != SKSE::RUNTIME_SSE_1_5_97) {
        logger::critical(
            "Unsupported Skyrim runtime {}. JM_ElementalArrows intentionally targets 1.5.97.",
            runtime.string());
        return false;
    }

    const auto* messaging = SKSE::GetMessagingInterface();
    if (!messaging) {
        logger::critical("SKSE messaging interface unavailable");
        return false;
    }

    if (!messaging->RegisterListener([](SKSE::MessagingInterface::Message* a_message) {
            if (!a_message) {
                return;
            }

            if (a_message->type == SKSE::MessagingInterface::kDataLoaded) {
                if (!JM::ElementalArrows::Initialize()) {
                    logger::critical("JM_ElementalArrows initialization failed");
                }
            }
        })) {
        logger::critical("Failed to register SKSE messaging listener");
        return false;
    }

    logger::info("JM_ElementalArrows loaded; waiting for data load");
    return true;
}
