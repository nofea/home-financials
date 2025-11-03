#include "home_manager.hpp"

/**
 * @brief Construct a new HomeManager object
 *
 * HomeManager holds an internal StorageManager instance and forwards
 * higher-level operations requested by the UI layer to StorageManager.
 */
HomeManager::HomeManager()
{
	ptr_storage = std::make_unique<StorageManager>();
}


/**
 * @brief Destroy the HomeManager object
 */
HomeManager::~HomeManager()
{
	ptr_storage.reset();
}


StorageManager::Result HomeManager::addFamily(const Family &family)
{
	uint64_t id = 0;
	return ptr_storage->saveFamilyDataEx(family, &id);
}


std::unique_ptr<Family> HomeManager::getFamily(const uint64_t family_id)
{
	Family* f = ptr_storage->getFamilyData(family_id);
	if (!f) return nullptr;
	return std::unique_ptr<Family>(f);
}


StorageManager::Result HomeManager::updateFamilyName(const uint64_t family_id, const std::string &new_name)
{
	return ptr_storage->updateFamilyDataEx(family_id, new_name);
}


StorageManager::Result HomeManager::deleteFamily(const uint64_t family_id)
{
	return ptr_storage->deleteFamilyDataEx(family_id);
}


StorageManager::Result HomeManager::addMemberToFamily(const Member &member, const uint64_t family_id)
{
	uint64_t id = 0;
	return ptr_storage->saveMemberDataEx(member, family_id, &id);
}


std::unique_ptr<Member> HomeManager::getMember(const uint64_t member_id)
{
	Member* m = ptr_storage->getMemberData(member_id);
	if (!m) return nullptr;
	return std::unique_ptr<Member>(m);
}


StorageManager::Result HomeManager::updateMember(const uint64_t member_id, const std::string &new_name, const std::string &new_nickname)
{
	return ptr_storage->updateMemberDataEx(member_id, new_name, new_nickname);
}


StorageManager::Result HomeManager::deleteMember(const uint64_t member_id)
{
	return ptr_storage->deleteMemberDataEx(member_id);
}
