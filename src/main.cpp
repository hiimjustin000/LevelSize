#include <numeric>
#include <Geode/Geode.hpp>

using namespace geode::prelude;

bool sortBySize = false;

std::string roundedDivision(int a, int b) {
    auto str = std::to_string(std::round(a * 100.0f / b) / 100.0f);
    auto found = str.find('.');
    return found != std::string::npos && str.substr(found).size() > 3 ? str.substr(0, found + 3) : str;
}

std::string getSizeString(int size) {
    if (size < 1024) return std::to_string(size) + " B";
    else if (size < 1048576) return roundedDivision(size, 1024) + " KB";
    else if (size < 1073741824) return roundedDivision(size, 1048576) + " MB";
    else return roundedDivision(size, 1073741824) + " GB";
}

int getTotalSize(CCArray* levelArray) {
    auto levels = CCArrayExt<GJGameLevel*>(levelArray);
    return std::accumulate(levels.begin(), levels.end(), 0, [](auto a, auto x) { return a + x->m_levelString.size(); });
}

#include <Geode/modify/LevelBrowserLayer.hpp>
class $modify(LSLevelBrowserLayer, LevelBrowserLayer) {
    bool init(GJSearchObject* searchObject) {
        if (!LevelBrowserLayer::init(searchObject)) return false;

        if (searchObject->m_searchType == SearchType::MyLevels) {
            auto winSize = CCDirector::sharedDirector()->getWinSize();

            auto sizeSortMenu = CCMenu::create();
            sizeSortMenu->setPosition(winSize.width / 2, winSize.height / 2 - 124.0f);
            sizeSortMenu->setID("size-sort-menu"_spr);
            addChild(sizeSortMenu, 1);
            
            auto spriteOff = ButtonSprite::create(CCSprite::create("LS_toggleBtn_001.png"_spr), 32, true, 32.0f, "GJ_button_01.png", 1.0f);
            spriteOff->setScale(0.5f);
            auto spriteOn = ButtonSprite::create(CCSprite::create("LS_toggleBtn_001.png"_spr), 32, true, 32.0f, "GJ_button_02.png", 1.0f);
            spriteOn->setScale(0.5f);
            auto sizeSortToggler = CCMenuItemToggler::create(spriteOff, spriteOn, this, menu_selector(LSLevelBrowserLayer::onSortBySize));
            sizeSortToggler->toggle(sortBySize);
            sizeSortToggler->setID("size-sort-toggler"_spr);
            sizeSortMenu->addChild(sizeSortToggler);

            if (Mod::get()->getSettingValue<bool>("show-total-size")) {
                auto totalSizeLabel = CCLabelBMFont::create(fmt::format("Total Size: {}", getSizeString(getTotalSize(m_list->m_listView->m_entries))).c_str(), "bigFont.fnt");
                totalSizeLabel->setScale(0.4f);
                totalSizeLabel->limitLabelWidth(130.0f, 0.4f, 0.0f);
                totalSizeLabel->setPosition(winSize.width / 2 + 95.0f, winSize.height / 2 - 122.0f);
                totalSizeLabel->setID("total-size-label"_spr);
                addChild(totalSizeLabel, 10);
            }
        }

        return true;
    }

    void sortLevelsBySize() {
        auto llm = LocalLevelManager::sharedState();
        auto levels = std::vector<GJGameLevel*>();
        for (auto level : CCArrayExt<GJGameLevel*>(llm->m_localLevels)) {
            levels.push_back(level);
        }
        if (sortBySize) std::sort(levels.begin(), levels.end(), [](auto a, auto b) { return a->m_levelString.size() > b->m_levelString.size(); });
        auto newArr = CCArray::create();
        for (int i = m_pageStartIdx; i < m_pageStartIdx + m_pageEndIdx && i < levels.size(); i++) {
            newArr->addObject(levels[i]);
        }
        LevelBrowserLayer::setupLevelBrowser(newArr);
    }

    void onSortBySize(CCObject*) {
        sortBySize = !sortBySize;
        sortLevelsBySize();
    }

    void setupLevelBrowser(CCArray* levelArray) {
        if (m_searchObject->m_searchType == SearchType::MyLevels) {
            if (sortBySize) sortLevelsBySize();
            else LevelBrowserLayer::setupLevelBrowser(levelArray);
            if (Mod::get()->getSettingValue<bool>("show-total-size")) {
                auto totalSizeLabel = static_cast<CCLabelBMFont*>(getChildByID("total-size-label"_spr));
                if (totalSizeLabel != nullptr) {
                    totalSizeLabel->setString(fmt::format("Total Size: {}", getSizeString(getTotalSize(m_list->m_listView->m_entries))).c_str());
                    totalSizeLabel->limitLabelWidth(130.0f, 0.4f, 0.0f);
                }
            }
        }
        else LevelBrowserLayer::setupLevelBrowser(levelArray);
    }
};

#include <Geode/modify/LevelCell.hpp>
class $modify(LSLevelCell, LevelCell) {
    void loadLocalLevelCell() {
        LevelCell::loadLocalLevelCell();

        if (Mod::get()->getSettingValue<bool>("show-size")) {
            auto sizeLabel = CCLabelBMFont::create(getSizeString(m_level->m_levelString.size()).c_str(), "goldFont.fnt");
            sizeLabel->setPosition(m_width - 6.0f, 3.0f);
            sizeLabel->setAnchorPoint({ 1.0f, 0.0f });
            sizeLabel->setScale(0.4f);
            sizeLabel->setID("size-label"_spr);
            m_mainLayer->addChild(sizeLabel);
        }
    }
};
