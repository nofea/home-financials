#pragma once

#include <gtest/gtest.h>
#include <filesystem>
#include <memory>

#include "storage_manager.hpp"
#include "home_manager.hpp"

// Small test helper fixture to reduce repeated DB setup/teardown across tests.
// Usage:
//   class MyTest : public TestDbFixture { ... };
//   In tests, call initStorage("unique_suffix") or initHome("unique_suffix") in SetUp
//   or let tests call them explicitly.

class TestDbFixture : public ::testing::Test
{
protected:
    void initStorage(const std::string &suffix = "")
    {
        tmp_path = std::filesystem::temp_directory_path() / (std::string("homefinancials_") + suffix + "_test.db");
        if (std::filesystem::exists(tmp_path))
        {
            std::filesystem::remove(tmp_path);
        }

        sm = std::make_unique<StorageManager>();
        ASSERT_TRUE(sm->initializeDatabase(tmp_path.string()));
    }

    void initHome(const std::string &suffix = "")
    {
        tmp_path = std::filesystem::temp_directory_path() / (std::string("homefinancials_") + suffix + "_test.db");
        if (std::filesystem::exists(tmp_path))
        {
            std::filesystem::remove(tmp_path);
        }

        hm = std::make_unique<HomeManager>();
        auto* s = hm->getStorageManager();
        ASSERT_TRUE(s->initializeDatabase(tmp_path.string()));
    }

    // Destroy only the StorageManager instance but keep the DB file on disk
    void destroyStorage()
    {
        sm.reset();
    }

    // Destroy only the HomeManager instance but keep the DB file on disk
    void destroyHome()
    {
        hm.reset();
    }

    void cleanup()
    {
        hm.reset();
        sm.reset();
        if (!tmp_path.empty() && std::filesystem::exists(tmp_path))
        {
            std::filesystem::remove(tmp_path);
        }
    }

    StorageManager* storage()
    {
        return sm.get();
    }

    HomeManager* home()
    {
        return hm.get();
    }

    std::filesystem::path tmp_path;

private:
    std::unique_ptr<StorageManager> sm;
    std::unique_ptr<HomeManager> hm;
};
