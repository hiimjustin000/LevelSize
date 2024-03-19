#include <Geode/Geode.hpp>
#include <Geode/modify/LevelBrowserLayer.hpp>
#include <Geode/modify/LevelCell.hpp>

using namespace geode::prelude;

bool sortBySize = false;

std::string roundedDivision(int a, int b) {
    std::string str = std::to_string(std::round(a * 100.0f / b) / 100.0f);
    return str.find('.') != std::string::npos && str.substr(str.find('.')).size() > 3 ? str.substr(0, str.find('.') + 3) : str;
}

std::string getSizeString(int size) {
    if (size < 1024) return std::to_string(size) + " B";
    else if (size < 1048576) return roundedDivision(size, 1024) + " KB";
    else if (size < 1073741824) return roundedDivision(size, 1048576) + " MB";
    else return roundedDivision(size, 1073741824) + " GB";
}

bool compareLevels(GJGameLevel* a, GJGameLevel* b) {
    return a->m_levelString.size() > b->m_levelString.size();
}

int getTotalSize(CCArray* levels) {
    int totalSize = 0;
    CCObject* obj;
    CCARRAY_FOREACH(levels, obj) {
        totalSize += static_cast<GJGameLevel*>(obj)->m_levelString.size();
    }
    return totalSize;
}

class $modify(LSLevelBrowserLayer, LevelBrowserLayer) {
    bool init(GJSearchObject* searchObject) {
        if (!LevelBrowserLayer::init(searchObject)) return false;

        if (searchObject->m_searchType == SearchType::MyLevels) {
            CCMenu* myLevelsMenu = static_cast<CCMenu*>(this->getChildByID("my-levels-menu"));
            CCLabelBMFont* selectAllText = static_cast<CCLabelBMFont*>(this->getChildByID("select-all-text"));
            CCMenu* sizeSortMenu = CCMenu::create();
            sizeSortMenu->setPosition(0.0f, 0.0f);
            sizeSortMenu->setID("size-sort-menu"_spr);
            CCMenuItemToggler* sizeSortToggler = CCMenuItemToggler::create(
                CCSprite::createWithSpriteFrameName("GJ_checkOff_001.png"),
                CCSprite::createWithSpriteFrameName("GJ_checkOn_001.png"),
                this,
                menu_selector(LSLevelBrowserLayer::onSortBySize)
            );
            sizeSortToggler->toggle(sortBySize);
            sizeSortToggler->setPosition(myLevelsMenu->getPositionX(), myLevelsMenu->getPositionY() + myLevelsMenu->getScaledContentSize().height / 2 - 60.0f);
            sizeSortToggler->setID("size-sort-toggler"_spr);
            CCLabelBMFont* sizeSortLabel = CCLabelBMFont::create("Sort by\nSize", "bigFont.fnt", kCCLabelAutomaticWidth, kCCTextAlignmentCenter);
            sizeSortLabel->setAnchorPoint({ 0.5f, 0.0f });
            sizeSortLabel->setScale(0.4f);
            sizeSortLabel->setPosition(sizeSortToggler->getPositionX(), sizeSortToggler->getPositionY() + sizeSortToggler->getScaledContentSize().height / 2 + 5.0f);
            sizeSortLabel->setID("size-sort-label"_spr);
            sizeSortMenu->addChild(sizeSortToggler);
            this->addChild(sizeSortMenu);
            this->addChild(sizeSortLabel);
            if (Mod::get()->getSettingValue<bool>("show-total-size")) {
                CCLabelBMFont* totalSizeLabel = CCLabelBMFont::create(("Total Size: " + getSizeString(getTotalSize(m_list->m_listView->m_entries))).c_str(), "bigFont.fnt");
                totalSizeLabel->setScale(0.4f);
                if (totalSizeLabel->getScaledContentSize().width > 130.0f) totalSizeLabel->setScale(52.0f / totalSizeLabel->getScaledContentSize().width);
                totalSizeLabel->setPosition(379.5f, selectAllText->getPositionY());
                totalSizeLabel->setZOrder(10);
                totalSizeLabel->setID("total-size-label"_spr);
                this->addChild(totalSizeLabel);
            }
        }

        return true;
    }

    void sortLevelsBySize(bool enabled) {
        CCObject* obj;
        LocalLevelManager* llm = LocalLevelManager::sharedState();
        std::vector<GJGameLevel*> levels;
        CCARRAY_FOREACH(llm->m_localLevels, obj) {
            levels.push_back(static_cast<GJGameLevel*>(obj));
        }
        if (enabled) std::sort(levels.begin(), levels.end(), compareLevels);
        CCArray* newArr = CCArray::create();
        for (int i = m_pageStartIdx; i < m_pageStartIdx + m_pageEndIdx && i < levels.size(); i++) {
            newArr->addObject(levels[i]);
        }
        LevelBrowserLayer::setupLevelBrowser(newArr);
    }

    void onSortBySize(CCObject* sender) {
        sortBySize = !sortBySize;
        sortLevelsBySize(sortBySize);
    }

    void setupLevelBrowser(CCArray* levelArray) {
        if (m_searchObject->m_searchType == SearchType::MyLevels) {
            if (sortBySize) sortLevelsBySize(true);
            else LevelBrowserLayer::setupLevelBrowser(levelArray);
            if (Mod::get()->getSettingValue<bool>("show-total-size")) {
                CCLabelBMFont* totalSizeLabel = static_cast<CCLabelBMFont*>(this->getChildByID("total-size-label"_spr));
                if (totalSizeLabel != nullptr) {
                    totalSizeLabel->setString(("Total Size: " + getSizeString(getTotalSize(m_list->m_listView->m_entries))).c_str());
                    if (totalSizeLabel->getScaledContentSize().width > 130.0f) totalSizeLabel->setScale(52.0f / totalSizeLabel->getScaledContentSize().width);
                }
            }
        }
        else LevelBrowserLayer::setupLevelBrowser(levelArray);
    }
};

class $modify(LSLevelCell, LevelCell) {
    void loadLocalLevelCell() {
        LevelCell::loadLocalLevelCell();

        if (Mod::get()->getSettingValue<bool>("show-size")) {
            CCLabelBMFont* nameLabel = static_cast<CCLabelBMFont*>(m_mainLayer->getChildByID("level-name"));
            CCNode* levelRevision = m_mainLayer->getChildByID("level-revision");
            CCLabelBMFont* revLabel = levelRevision != nullptr ? static_cast<CCLabelBMFont*>(levelRevision) : nullptr;
            CCLabelBMFont* sizeLabel = CCLabelBMFont::create(getSizeString(m_level->m_levelString.size()).c_str(), "bigFont.fnt");
            sizeLabel->setAnchorPoint({ 0.0f, 0.0f });
            sizeLabel->setScale(0.3f);
            if (sizeLabel->getScaledContentSize().width > 30.0f && nameLabel->getContentWidth() * 0.8f >= 190.0f)
                sizeLabel->setScale(9.0f / sizeLabel->getScaledContentSize().width);
            sizeLabel->setPosition(nameLabel->getPositionX() + nameLabel->getScaledContentSize().width + 10.0f,
                revLabel != nullptr ? revLabel->getScaledContentSize().height + revLabel->getPositionY() :
                nameLabel->getPositionY() - nameLabel->getScaledContentSize().height / 2 + 1.0f);
            sizeLabel->setID("size-label"_spr);
            m_mainLayer->addChild(sizeLabel);
        }
    }
};
