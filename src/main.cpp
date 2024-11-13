using namespace geode::prelude;

bool localSortBySize = false;
bool savedSortBySize = false;

std::string getSizeString(size_t size) {
    auto divisor = 0;
    auto suffix = "";
    if (size < 1024) {
        divisor = 1;
        suffix = " B";
    }
    else if (size < 1048576) {
        divisor = 1024;
        suffix = " KB";
    }
    else if (size < 1073741824) {
        divisor = 1048576;
        suffix = " MB";
    }
    else {
        divisor = 1073741824;
        suffix = " GB";
    }
    return divisor == 1 ? fmt::format("{}{}", size, suffix) : fmt::format("{:.2f}{}", (float)size / divisor, suffix);
}

size_t getTotalSize(CCArray* levels) {
    size_t totalSize = 0;
    for (auto level : CCArrayExt<GJGameLevel*>(levels)) {
        totalSize += level->m_levelString.size();
    }
    return totalSize;
}

#include <Geode/modify/LevelBrowserLayer.hpp>
class $modify(LSLevelBrowserLayer, LevelBrowserLayer) {
    struct Fields {
        CCLabelBMFont* m_totalSizeLabel;
        CCLabelBMFont* m_overallSizeLabel;
    };

    bool init(GJSearchObject* searchObject) {
        if (!LevelBrowserLayer::init(searchObject)) return false;

        auto searchType = searchObject->m_searchType;
        if (searchType != SearchType::MyLevels && searchType != SearchType::SavedLevels) return true;

        auto winSize = CCDirector::sharedDirector()->getWinSize();

        auto sizeSortMenu = CCMenu::create();
        sizeSortMenu->setPosition(winSize / 2 - CCPoint { 0.0f, 124.0f });
        sizeSortMenu->setID("size-sort-menu"_spr);
        addChild(sizeSortMenu, 1);

        auto sizeSortSprite = ButtonSprite::create(CCSprite::create("LS_toggleBtn_001.png"_spr), 32, true, 32.0f,
            (searchType == SearchType::MyLevels && localSortBySize) || (searchType == SearchType::SavedLevels && savedSortBySize)
                ? "GJ_button_02.png" : "GJ_button_01.png", 1.0f);
        sizeSortSprite->setScale(0.5f);
        auto sizeSortToggler = CCMenuItemSpriteExtra::create(sizeSortSprite, this, menu_selector(LSLevelBrowserLayer::onSizeSort));
        sizeSortToggler->setID("size-sort-toggler"_spr);
        sizeSortMenu->addChild(sizeSortToggler);

        auto f = m_fields.self();
        if (Mod::get()->getSettingValue<bool>("show-total-size")) {
            f->m_totalSizeLabel = CCLabelBMFont::create(fmt::format("Total Size: {}",
                getSizeString(getTotalSize(m_list->m_listView->m_entries))).c_str(), "bigFont.fnt");
            f->m_totalSizeLabel->setScale(0.4f);
            f->m_totalSizeLabel->limitLabelWidth(130.0f, 0.4f, 0.0f);
            f->m_totalSizeLabel->setPosition(winSize / 2 + CCPoint { 95.0f, -116.0f });
            f->m_totalSizeLabel->setID("total-size-label"_spr);
            addChild(f->m_totalSizeLabel, 10);
        }
        if (Mod::get()->getSettingValue<bool>("show-overall-size")) {
            f->m_overallSizeLabel = CCLabelBMFont::create(fmt::format("Overall Size: {}",
                getSizeString(getTotalSize(getLevelArray()))).c_str(), "bigFont.fnt");
            f->m_overallSizeLabel->setScale(0.4f);
            f->m_overallSizeLabel->limitLabelWidth(130.0f, 0.4f, 0.0f);
            f->m_overallSizeLabel->setPosition(winSize / 2 + CCPoint { 95.0f, -127.0f });
            f->m_overallSizeLabel->setID("overall-size-label"_spr);
            addChild(f->m_overallSizeLabel, 10);
        }

        return true;
    }

    void onSizeSort(CCObject* sender) {
        auto sizeSortSprite = static_cast<ButtonSprite*>(static_cast<CCMenuItemSpriteExtra*>(sender)->getNormalImage());
        auto sortBySize = false;
        switch (m_searchObject->m_searchType) {
            case SearchType::MyLevels:
                localSortBySize = !localSortBySize;
                sortBySize = localSortBySize;
                log::info("Local sort by size: {}", localSortBySize);
                break;
            case SearchType::SavedLevels:
                savedSortBySize = !savedSortBySize;
                sortBySize = savedSortBySize;
                log::info("Saved sort by size: {}", savedSortBySize);
                break;
            default:
                break;
        }

        sizeSortSprite->updateBGImage(sortBySize ? "GJ_button_02.png" : "GJ_button_01.png");
        static_cast<CCMenuItemSpriteExtra*>(sender)->updateSprite();

        sortLevelsBySize();
    }

    CCArray* getLevelArray() {
        switch (m_searchObject->m_searchType) {
            case SearchType::MyLevels:
                return LocalLevelManager::sharedState()->getCreatedLevels(m_searchObject->m_folder);
            case SearchType::SavedLevels:
                return GameLevelManager::sharedState()->getSavedLevels(false, m_searchObject->m_folder);
            default:
                return CCArray::create();
        }
    }

    void sortLevelsBySize() {
        auto levels = getLevelArray(); // I spent two hours trying to fix my save file after not copying the array
        auto levelsCopy = CCArray::create();
        levelsCopy->addObjectsFromArray(levels);

        auto searchType = m_searchObject->m_searchType;
        if ((searchType == SearchType::MyLevels && localSortBySize) ||
            (searchType == SearchType::SavedLevels && savedSortBySize))
            qsort(levelsCopy->data->arr, levelsCopy->data->num, sizeof(GJGameLevel*), [](const void* a, const void* b) {
                auto sizeA = (*reinterpret_cast<GJGameLevel* const*>(a))->m_levelString.size();
                auto sizeB = (*reinterpret_cast<GJGameLevel* const*>(b))->m_levelString.size();
                return (sizeA < sizeB) - (sizeA > sizeB);
            });

        auto newArr = CCArray::create();
        for (int i = m_pageStartIdx; i < m_pageStartIdx + m_pageEndIdx && i < m_itemCount; i++) {
            newArr->addObject(levelsCopy->objectAtIndex(i));
        }

        LevelBrowserLayer::setupLevelBrowser(newArr);
    }

    void setupLevelBrowser(CCArray* levels) {
        auto searchType = m_searchObject->m_searchType;
        if (searchType != SearchType::MyLevels && searchType != SearchType::SavedLevels) {
            LevelBrowserLayer::setupLevelBrowser(levels);
            return;
        }

        if ((searchType == SearchType::MyLevels && localSortBySize) || (searchType == SearchType::SavedLevels && savedSortBySize))
            sortLevelsBySize();
        else LevelBrowserLayer::setupLevelBrowser(levels);

        auto f = m_fields.self();
        if (Mod::get()->getSettingValue<bool>("show-total-size") && f->m_totalSizeLabel) {
            f->m_totalSizeLabel->setString(fmt::format("Total Size: {}", getSizeString(getTotalSize(m_list->m_listView->m_entries))).c_str());
            f->m_totalSizeLabel->limitLabelWidth(130.0f, 0.4f, 0.0f);
        }
        if (Mod::get()->getSettingValue<bool>("show-overall-size") && f->m_overallSizeLabel) {
            f->m_overallSizeLabel->setString(fmt::format("Overall Size: {}", getSizeString(getTotalSize(getLevelArray()))).c_str());
            f->m_overallSizeLabel->limitLabelWidth(130.0f, 0.4f, 0.0f);
        }
    }
};

#include <Geode/modify/LevelCell.hpp>
class $modify(LSLevelCell, LevelCell) {
    void loadFromLevel(GJGameLevel* level) {
        LevelCell::loadFromLevel(level);

        if (!Mod::get()->getSettingValue<bool>("show-size")) return;

        switch (level->m_levelType) {
            case GJLevelType::Editor: {
                auto sizeLabel = CCLabelBMFont::create(getSizeString(m_level->m_levelString.size()).c_str(), "goldFont.fnt");
                sizeLabel->setPosition({ 350.0f, 3.0f });
                sizeLabel->setScale(0.4f);
                sizeLabel->setAnchorPoint({ 1.0f, 0.0f });
                sizeLabel->setID("size-label"_spr);
                m_mainLayer->addChild(sizeLabel);
                break;
            }
            case GJLevelType::Saved: {
                auto sizeLabel = CCLabelBMFont::create(getSizeString(level->m_levelString.size()).c_str(), "chatFont.fnt");
                auto levelRankLabel = m_mainLayer->getChildByID("hiimjustin000.integrated_demonlist/level-rank-label");
                sizeLabel->setPosition({
                    346.0f - (m_compactView && levelRankLabel ? levelRankLabel->getScaledContentWidth() + 3.0f : 0.0f),
                    !m_compactView && levelRankLabel ? 12.0f : 1.0f
                });
                sizeLabel->setScale(m_compactView ? 0.45f : 0.6f);
                auto whiteSize = Mod::get()->getSettingValue<bool>("white-size");
                sizeLabel->setColor(whiteSize ? ccColor3B { 255, 255, 255 } : ccColor3B { 51, 51, 51 });
                sizeLabel->setOpacity(whiteSize ? 200 : 152);
                sizeLabel->setAnchorPoint({ 1.0f, 0.0f });
                sizeLabel->setID("size-label"_spr);
                m_mainLayer->addChild(sizeLabel);
                break;
            }
            default:
                break;
        }
    }
};
