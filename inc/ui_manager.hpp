#pragma once

#include <string>
#include <vector>
#include <cstdint>

#include "commons.hpp"
#include "home_manager.hpp"

class UIManager
{
public:
    UIManager();
    virtual ~UIManager();

    // Core abstract operations that concrete UI managers must implement
    virtual commons::Result addFamily(const std::string& name) = 0;
    virtual commons::Result deleteFamily(uint64_t family_id) = 0;

    virtual commons::Result addMember(uint64_t family_id, const Member& member) = 0;
    virtual commons::Result updateMember(uint64_t family_id,
                                        uint64_t member_id,
                                        const std::string& new_name,
                                        const std::string& new_nickname) = 0;

    virtual commons::Result deleteMember(uint64_t family_id, uint64_t member_id) = 0;
    virtual commons::Result deleteMembers(uint64_t family_id, const std::vector<uint64_t>& member_ids) = 0;

    // Shared helper: present a clear message for a commons::Result value.
    // This is implemented in the base class so derived classes can reuse it.
    void showError(commons::Result res);

protected:
    // Translate an error code to a human-friendly message.
    static std::string errorMessage(commons::Result res);
};
