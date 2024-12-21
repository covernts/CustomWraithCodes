#include <Geode/Geode.hpp>
#include <Geode/modify/SecretLayer5.hpp>
#include <Geode/modify/GameStatsManager.hpp>
#include <Geode/utils/string.hpp>

using namespace geode::prelude;
using namespace cocos2d;

const char* getStatId(SpecialRewardItem type) {
	switch (type) {
		case SpecialRewardItem::Orbs: return "14";
		case SpecialRewardItem::BonusKey: return "21";
		case SpecialRewardItem::Diamonds: return "13";
		case SpecialRewardItem::GoldKey: return "43";
		default: return "0";
	}
}

std::string removeSpaces(std::string input) {
	input.erase(std::remove(input.begin(), input.end(), ' '), input.end());
	return input;
}

GJRewardObject* create_reward(std::string type, int64_t amount) {
	auto gsm = GameStatsManager::sharedState();
	auto obj = GJRewardObject::create();

	if (type == "Orbs") {
		gsm->incrementStat(getStatId(SpecialRewardItem::Orbs), amount);
		obj->m_specialRewardItem = SpecialRewardItem::Orbs;
		obj->m_total = amount;
	} else if (type == "Keys") {
		gsm->incrementStat(getStatId(SpecialRewardItem::BonusKey), amount);
		obj->m_specialRewardItem = SpecialRewardItem::BonusKey;
		obj->m_total = amount;
	} else if (type == "Diamonds") {
		gsm->incrementStat(getStatId(SpecialRewardItem::Diamonds), amount);
		obj->m_specialRewardItem = SpecialRewardItem::Diamonds;
		obj->m_total = amount;
	} else if (type == "Gold Keys") {
		gsm->incrementStat(getStatId(SpecialRewardItem::GoldKey), amount);
		obj->m_specialRewardItem = SpecialRewardItem::GoldKey;
		obj->m_total = amount;
	}
	
	return obj;
}

class $modify(CustomWraithCodes, SecretLayer5) {
	struct Fields {
		std::string text = "";
	};

	static void onModify(auto& self) {
		if (!self.setHookPriorityPre("SecretLayer5::onlineRewardStatusFailed", Priority::First)) {
			log::error("Error setting hook priority for: SecretLayer5::onlineRewardStatusFailed");
		}
	}

	bool init() {
		if (!SecretLayer5::init()) return false;

		if (!Mod::get()->getSavedValue("disclaimer", false)) {
			auto popup = FLAlertLayer::create("Custom Wraith Codes", "This mod uses an <cy>anticheat bypass</c> which can get you leaderboard <cr>BANNED</c>. Safe mode is on by default. Safe mode can be disabled in the mod settings. Proceed with caution.", "OK");
			popup->m_scene = this;
			popup->show();
			Mod::get()->setSavedValue("disclaimer", true);
		}

		return true;
	}

	void onSubmit(CCObject* sender) {
		m_fields->text = utils::string::toLower(this->m_textInput->getString());
		SecretLayer5::onSubmit(sender);
	}

    void onlineRewardStatusFailed() {
        auto code = removeSpaces(utils::string::toLower(Mod::get()->getSettingValue<std::string>("code")));
		
		if (removeSpaces(m_fields->text) == code) {
			auto sequence = CCSequence::create(
				CCDelayTime::create(4),
				CCCallFunc::create(this, callfunc_selector(CustomWraithCodes::showSuccess_W)),
				CCDelayTime::create(1),
				CCCallFunc::create(this, callfunc_selector(CustomWraithCodes::fadeLoadingCircle_W)),
				CCCallFunc::create(this, callfunc_selector(CustomWraithCodes::showReward_W)),
				nullptr
			);

			CCDirector::sharedDirector()->getRunningScene()->runAction(sequence);

			this->m_wraithButton->setEnabled(true);
		} else {
			SecretLayer5::onlineRewardStatusFailed();
		}
	}

	// Wrappers
    void showSuccess_W() {
		this->showSuccessAnimation();
	}

	void fadeLoadingCircle_W() {
		this->m_circleSprite->fadeInCircle(false, 0.5f, 0.f);
	}

	void showReward_W() {
		auto gsm = GameStatsManager::sharedState();
		
		auto type = Mod::get()->getSettingValue<std::string>("reward-type");
		auto amount = Mod::get()->getSettingValue<int64_t>("reward-amount");
		auto safe = Mod::get()->getSettingValue<bool>("safe-mode");

		auto rewardObj = create_reward(type, amount);
		
		if (rewardObj) {
			auto item = GJRewardItem::createWithObject(GJRewardType::Unknown, rewardObj);
			auto layer = RewardUnlockLayer::create(1, nullptr);

			auto scene = CCDirector::sharedDirector()->getRunningScene();
			scene->addChild(layer);
			layer->showCollectReward(item);

			auto call = CCCallFunc::create(this, nullptr);
			scene->runAction(CCSequence::create(CCDelayTime::create(2), call, nullptr));

			if (safe && rewardObj->m_specialRewardItem != SpecialRewardItem::CustomItem) 
				gsm->incrementStat(getStatId(rewardObj->m_specialRewardItem), -amount);
		} else {
			FLAlertLayer::create("Error", "There was an error processing your reward.", "OK")->show();
		}
	}
};