#include "storage_manager.hpp"

int main() 
{
    StorageManager storageManager;
    storageManager.initializeDatabase("path/to/database");

    Member member1(1, "John Doe", "Johnny");
    Family family("Doe Family");
    family.addMember(member1);

    storageManager.saveMemberData(member1);
    storageManager.saveFamilyData(family);

    return 0;
}