#pragma once

#include "commons.hpp"
#include "storage_manager.hpp"
#include <memory>
#include <string>
#include <cstdint>

/**
 * HomeManager acts as an interface between the UI layer and StorageManager.
 * It exposes higher-level operations used by the UI: add/update/delete
 * families and members and simple retrieval helpers.
 */
class HomeManager
{
public:
    HomeManager();
    ~HomeManager();

    // Family operations (return StorageManager::Result for error reporting)
    StorageManager::Result addFamily(const Family &family);
    std::unique_ptr<Family> getFamily(const uint64_t family_id);
    StorageManager::Result updateFamilyName(const uint64_t family_id, const std::string &new_name);
    StorageManager::Result deleteFamily(const uint64_t family_id);

    // Member operations
    StorageManager::Result addMemberToFamily(const Member &member, const uint64_t family_id);
    std::unique_ptr<Member> getMember(const uint64_t member_id);
    StorageManager::Result updateMember(const uint64_t member_id, const std::string &new_name, const std::string &new_nickname);
    StorageManager::Result deleteMember(const uint64_t member_id);

    // Testing access
    StorageManager* getStorageManager() { return ptr_storage.get(); }

private:
    std::unique_ptr<StorageManager> ptr_storage;
};
