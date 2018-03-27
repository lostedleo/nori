// libraft - Quorum-based replication of states across machines.
// Copyright (c) 2015 Baidu.com, Inc. All Rights Reserved

// Author: WangYao (fisherman), wangyao02@baidu.com
// Date: 2015/10/08 17:00:05

#include <gtest/gtest.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <butil/atomicops.h>
#include <butil/file_util.h>
#include <butil/files/file_path.h>
#include <butil/files/file_enumerator.h>
#include <butil/string_printf.h>
#include <butil/logging.h>
#include "braft/log.h"

class LogStorageTest : public testing::Test {
protected:
    void SetUp() {
        braft::FLAGS_raft_sync = false;
    }
    void TearDown() {}
};

TEST_F(LogStorageTest, open_segment) {
    // open segment operation
    ::system("mkdir data/");
    braft::Segment* seg1 = new braft::Segment("./data", 1L, 0);

    // not open
    braft::LogEntry* entry = seg1->get(1);
    ASSERT_TRUE(entry == NULL);

    // create and open
    ASSERT_EQ(0, seg1->create());
    ASSERT_TRUE(seg1->is_open());

    // append entry
    for (int i = 0; i < 10; i++) {
        braft::LogEntry* entry = new braft::LogEntry();
        entry->AddRef();
        entry->type = braft::ENTRY_TYPE_DATA;
        entry->id.term = 1;
        entry->id.index = i + 1;

        char data_buf[128];
        snprintf(data_buf, sizeof(data_buf), "hello, world: %d", i + 1);
        entry->data.append(data_buf);

        ASSERT_EQ(0, seg1->append(entry));

        entry->Release();
    }

    // read entry
    for (int i = 0; i < 10; i++) {
        int64_t term = seg1->get_term(i+1);
        ASSERT_EQ(term, 1);

        braft::LogEntry* entry = seg1->get(i+1);
        ASSERT_EQ(entry->id.term, 1);
        ASSERT_EQ(entry->type, braft::ENTRY_TYPE_DATA);
        ASSERT_EQ(entry->id.index, i+1);

        char data_buf[128];
        snprintf(data_buf, sizeof(data_buf), "hello, world: %d", i + 1);
        ASSERT_EQ(data_buf, entry->data.to_string());
        entry->Release();
    }
    {
        braft::LogEntry* entry = seg1->get(0);
        ASSERT_TRUE(entry == NULL);
        entry = seg1->get(11);
        ASSERT_TRUE(entry == NULL);
    }

    braft::ConfigurationManager* configuration_manager = new braft::ConfigurationManager;
    // load open segment
    braft::Segment* seg2 = new braft::Segment("./data", 1, 0);
    ASSERT_EQ(0, seg2->load(configuration_manager));

    for (int i = 0; i < 10; i++) {
        braft::LogEntry* entry = seg2->get(i+1);
        ASSERT_EQ(entry->id.term, 1);
        ASSERT_EQ(entry->type, braft::ENTRY_TYPE_DATA);
        ASSERT_EQ(entry->id.index, i+1);

        char data_buf[128];
        snprintf(data_buf, sizeof(data_buf), "hello, world: %d", i + 1);
        ASSERT_EQ(data_buf, entry->data.to_string());
        entry->Release();
    }
    {
        braft::LogEntry* entry = seg2->get(0);
        ASSERT_TRUE(entry == NULL);
        entry = seg2->get(11);
        ASSERT_TRUE(entry == NULL);
    }
    delete seg2;

    // truncate and read
    ASSERT_EQ(0, seg1->truncate(5));
    for (int i = 0; i < 5; i++) {
        braft::LogEntry* entry = new braft::LogEntry();
        entry->type = braft::ENTRY_TYPE_DATA;
        entry->id.term = 1;
        entry->id.index = i + 6;

        char data_buf[128];
        snprintf(data_buf, sizeof(data_buf), "HELLO, WORLD: %d", i + 6);
        entry->data.append(data_buf); 

        ASSERT_EQ(0, seg1->append(entry));

        entry->Release();
    }
    for (int i = 0; i < 10; i++) {
        braft::LogEntry* entry = seg1->get(i+1);
        ASSERT_EQ(entry->id.term, 1);
        ASSERT_EQ(entry->type, braft::ENTRY_TYPE_DATA);
        ASSERT_EQ(entry->id.index, i+1);

        char data_buf[128];
        if (i < 5) {
            snprintf(data_buf, sizeof(data_buf), "hello, world: %d", i + 1);
        } else {
            snprintf(data_buf, sizeof(data_buf), "HELLO, WORLD: %d", i + 1);
        }
        ASSERT_EQ(data_buf, entry->data.to_string());
        entry->Release();
    }

    ASSERT_EQ(0, seg1->close());
    ASSERT_FALSE(seg1->is_open());
    ASSERT_EQ(0, seg1->unlink());

    delete configuration_manager;
}

TEST_F(LogStorageTest, closed_segment) {
    // open segment operation
    braft::Segment* seg1 = new braft::Segment("./data", 1L, 0);
    ASSERT_EQ(0, seg1->create());
    ASSERT_TRUE(seg1->is_open());
    // append entry
    for (int i = 0; i < 10; i++) {
        braft::LogEntry* entry = new braft::LogEntry();
        entry->type = braft::ENTRY_TYPE_DATA;
        entry->id.term = 1;
        entry->id.index = i + 1;

        char data_buf[128];
        snprintf(data_buf, sizeof(data_buf), "hello, world: %d", i + 1);
        entry->data.append(data_buf);

        ASSERT_EQ(0, seg1->append(entry));

        entry->Release();
    }
    seg1->close();

    // read entry
    for (int i = 0; i < 10; i++) {
        braft::LogEntry* entry = seg1->get(i+1);
        ASSERT_EQ(entry->id.term, 1);
        ASSERT_EQ(entry->type, braft::ENTRY_TYPE_DATA);
        ASSERT_EQ(entry->id.index, i+1);

        char data_buf[128];
        snprintf(data_buf, sizeof(data_buf), "hello, world: %d", i + 1);
        ASSERT_EQ(data_buf, entry->data.to_string());
        entry->Release();
    }
    {
        braft::LogEntry* entry = seg1->get(0);
        ASSERT_TRUE(entry == NULL);
        entry = seg1->get(11);
        ASSERT_TRUE(entry == NULL);
    }

    braft::ConfigurationManager* configuration_manager = new braft::ConfigurationManager;
    // load open segment
    braft::Segment* seg2 = new braft::Segment("./data", 1, 10, 0);
    ASSERT_EQ(0, seg2->load(configuration_manager));

    for (int i = 0; i < 10; i++) {
        braft::LogEntry* entry = seg2->get(i+1);
        ASSERT_EQ(entry->id.term, 1);
        ASSERT_EQ(entry->type, braft::ENTRY_TYPE_DATA);
        ASSERT_EQ(entry->id.index, i+1);

        char data_buf[128];
        snprintf(data_buf, sizeof(data_buf), "hello, world: %d", i + 1);
        ASSERT_EQ(data_buf, entry->data.to_string());
        entry->Release();
    }
    {
        braft::LogEntry* entry = seg2->get(0);
        ASSERT_TRUE(entry == NULL);
        entry = seg2->get(11);
        ASSERT_TRUE(entry == NULL);
    }
    delete seg2;

    // truncate and read
    ASSERT_EQ(0, seg1->truncate(5));
    for (int i = 0; i < 5; i++) {
        braft::LogEntry* entry = new braft::LogEntry();
        entry->type = braft::ENTRY_TYPE_DATA;
        entry->id.term = 1;
        entry->id.index = i + 6;

        char data_buf[128];
        snprintf(data_buf, sizeof(data_buf), "HELLO, WORLD: %d", i + 6);
        entry->data.append(data_buf);

        ASSERT_NE(0, seg1->append(entry));

        entry->Release();
    }
    for (int i = 0; i < 10; i++) {
        braft::LogEntry* entry = seg1->get(i+1);
        char data_buf[128];
        if (i < 5) {
            snprintf(data_buf, sizeof(data_buf), "hello, world: %d", i + 1);
        } else {
            snprintf(data_buf, sizeof(data_buf), "HELLO, WORLD: %d", i + 1);
            ASSERT_TRUE(entry == NULL);
            continue;
        }
        ASSERT_EQ(entry->id.term, 1);
        ASSERT_EQ(entry->type, braft::ENTRY_TYPE_DATA);
        ASSERT_EQ(entry->id.index, i+1);
        ASSERT_EQ(data_buf, entry->data.to_string());
        entry->Release();
    }

    ASSERT_EQ(0, seg1->unlink());

    delete configuration_manager;
}

TEST_F(LogStorageTest, multi_segment_and_segment_logstorage) {
    ::system("rm -rf data");
    braft::SegmentLogStorage* storage = new braft::SegmentLogStorage("./data");

    // init
    ASSERT_EQ(0, storage->init(new braft::ConfigurationManager()));
    ASSERT_EQ(1, storage->first_log_index());
    ASSERT_EQ(0, storage->last_log_index());

    // append entry
    for (int i = 0; i < 100000; i++) {
        std::vector<braft::LogEntry*> entries;
        for (int j = 0; j < 5; j++) {
            int64_t index = 5*i + j + 1;
            braft::LogEntry* entry = new braft::LogEntry();
            entry->type = braft::ENTRY_TYPE_DATA;
            entry->id.term = 1;
            entry->id.index = index;

            char data_buf[128];
            snprintf(data_buf, sizeof(data_buf), "hello, world: %ld", index);
            entry->data.append(data_buf);
            entries.push_back(entry);
        }

        ASSERT_EQ(5, storage->append_entries(entries));

        for (size_t j = 0; j < entries.size(); j++) {
            entries[j]->Release();
        }
    }

    // read entry
    for (int i = 0; i < 500000; i++) {
        int64_t index = i + 1;
        braft::LogEntry* entry = storage->get_entry(index);
        ASSERT_EQ(entry->id.term, 1);
        ASSERT_EQ(entry->type, braft::ENTRY_TYPE_DATA);
        ASSERT_EQ(entry->id.index, index);

        char data_buf[128];
        snprintf(data_buf, sizeof(data_buf), "hello, world: %ld", index);
        ASSERT_EQ(data_buf, entry->data.to_string());
        entry->Release();
    }

    ASSERT_EQ(storage->first_log_index(), 1);
    ASSERT_EQ(storage->last_log_index(), 500000);
    // truncate prefix
    ASSERT_EQ(0, storage->truncate_prefix(10001));
    ASSERT_EQ(storage->first_log_index(), 10001);
    ASSERT_EQ(storage->last_log_index(), 500000);

    // boundary truncate prefix
    {
        braft::SegmentLogStorage::SegmentMap& segments1 = storage->segments();
        size_t old_segment_num = segments1.size();
        braft::Segment* first_seg = segments1.begin()->second.get();

        ASSERT_EQ(0, storage->truncate_prefix(first_seg->last_index()));
        braft::SegmentLogStorage::SegmentMap& segments2 = storage->segments();
        ASSERT_EQ(old_segment_num, segments2.size());

        ASSERT_EQ(0, storage->truncate_prefix(first_seg->last_index() + 1));
        braft::SegmentLogStorage::SegmentMap& segments3 = storage->segments();
        ASSERT_EQ(old_segment_num - 1, segments3.size());
    }

    ASSERT_EQ(0, storage->truncate_prefix(250001));
    ASSERT_EQ(storage->first_log_index(), 250001);
    ASSERT_EQ(storage->last_log_index(), 500000);
    for (int i = 250001; i <= 500000; i++) {
        int64_t index = i;
        braft::LogEntry* entry = storage->get_entry(index);
        ASSERT_EQ(entry->id.term, 1);
        ASSERT_EQ(entry->type, braft::ENTRY_TYPE_DATA);
        ASSERT_EQ(entry->id.index, index);

        char data_buf[128];
        snprintf(data_buf, sizeof(data_buf), "hello, world: %ld", index);
        ASSERT_EQ(data_buf, entry->data.to_string());
        entry->Release();
    }

    // append
    for (int i = 100000; i < 200000; i++) {
        std::vector<braft::LogEntry*> entries;
        for (int j = 0; j < 5; j++) {
            int64_t index = 5*i + j + 1;
            braft::LogEntry* entry = new braft::LogEntry();
            entry->type = braft::ENTRY_TYPE_DATA;
            entry->id.term = 1;
            entry->id.index = index;

            char data_buf[128];
            snprintf(data_buf, sizeof(data_buf), "hello, world: %ld", index);
            entry->data.append(data_buf);
            entries.push_back(entry);
        }

        ASSERT_EQ(5, storage->append_entries(entries));

        for (size_t j = 0; j < entries.size(); j++) {
            delete entries[j];
        }
    }

    // truncate suffix
    ASSERT_EQ(250001, storage->first_log_index());
    ASSERT_EQ(1000000, storage->last_log_index());
    ASSERT_EQ(0, storage->truncate_suffix(750000));
    ASSERT_EQ(250001, storage->first_log_index());
    ASSERT_EQ(750000, storage->last_log_index());

    // boundary truncate suffix
    {
        braft::SegmentLogStorage::SegmentMap& segments1 = storage->segments();
        braft::Segment* first_seg = segments1.begin()->second.get();
        if (segments1.size() > 1) {
            storage->truncate_suffix(first_seg->last_index() + 1);
        }
        braft::SegmentLogStorage::SegmentMap& segments2 = storage->segments();
        ASSERT_EQ(2ul, segments2.size());
        ASSERT_EQ(storage->last_log_index(), first_seg->last_index() + 1);
        storage->truncate_suffix(first_seg->last_index());
        braft::SegmentLogStorage::SegmentMap& segments3 = storage->segments();
        ASSERT_EQ(1ul, segments3.size());
        ASSERT_EQ(storage->last_log_index(), first_seg->last_index());
    }

    // read
    for (int i = 250001; i <= storage->last_log_index(); i++) {
        int64_t index = i;
        braft::LogEntry* entry = storage->get_entry(index);
        ASSERT_EQ(entry->id.term, 1);
        ASSERT_EQ(entry->type, braft::ENTRY_TYPE_DATA);
        ASSERT_EQ(entry->id.index, index);

        char data_buf[128];
        snprintf(data_buf, sizeof(data_buf), "hello, world: %ld", index);
        ASSERT_EQ(data_buf, entry->data.to_string());
        entry->Release();
    }

    delete storage;

    // re load
    ::system("rm -rf data/log_meta");
    braft::SegmentLogStorage* storage2 = new braft::SegmentLogStorage("./data");
    ASSERT_EQ(0, storage2->init(new braft::ConfigurationManager()));
    ASSERT_EQ(1, storage2->first_log_index());
    ASSERT_EQ(0, storage2->last_log_index());
    delete storage2;
}

TEST_F(LogStorageTest, append_close_load_append) {
    ::system("rm -rf data");
    braft::LogStorage* storage = new braft::SegmentLogStorage("./data");
    braft::ConfigurationManager* configuration_manager = new braft::ConfigurationManager;
    ASSERT_EQ(0, storage->init(configuration_manager));

    // append entry
    for (int i = 0; i < 100000; i++) {
        std::vector<braft::LogEntry*> entries;
        for (int j = 0; j < 5; j++) {
            int64_t index = 5*i + j + 1;
            braft::LogEntry* entry = new braft::LogEntry();
            entry->type = braft::ENTRY_TYPE_DATA;
            entry->id.term = 1;
            entry->id.index = index;

            char data_buf[128];
            snprintf(data_buf, sizeof(data_buf), "hello, world: %ld", index);
            entry->data.append(data_buf);
            entries.push_back(entry);
        }

        ASSERT_EQ(5, storage->append_entries(entries));

        for (size_t j = 0; j < entries.size(); j++) {
            delete entries[j];
        }
    }

    delete storage;
    delete configuration_manager;

    // reinit 
    storage = new braft::SegmentLogStorage("./data");
    configuration_manager = new braft::ConfigurationManager;
    ASSERT_EQ(0, storage->init(configuration_manager));

    // append entry
    for (int i = 100000; i < 200000; i++) {
        std::vector<braft::LogEntry*> entries;
        for (int j = 0; j < 5; j++) {
            int64_t index = 5*i + j + 1;
            braft::LogEntry* entry = new braft::LogEntry();
            entry->type = braft::ENTRY_TYPE_DATA;
            entry->id.term = 2;
            entry->id.index = index;

            char data_buf[128];
            snprintf(data_buf, sizeof(data_buf), "hello, world: %ld", index);
            entry->data.append(data_buf);
            entries.push_back(entry);
        }

        ASSERT_EQ(5, storage->append_entries(entries));

        for (size_t j = 0; j < entries.size(); j++) {
            delete entries[j];
        }
    }

    // check and read
    ASSERT_EQ(storage->first_log_index(), 1);
    ASSERT_EQ(storage->last_log_index(), 200000*5);

    for (int i = 0; i < 200000*5; i++) {
        int64_t index = i + 1;
        braft::LogEntry* entry = storage->get_entry(index);
        if (i < 100000*5) {
            ASSERT_EQ(entry->id.term, 1);
        } else {
            ASSERT_EQ(entry->id.term, 2);
        }
        ASSERT_EQ(entry->type, braft::ENTRY_TYPE_DATA);
        ASSERT_EQ(entry->id.index, index);

        char data_buf[128];
        snprintf(data_buf, sizeof(data_buf), "hello, world: %ld", index);
        ASSERT_EQ(data_buf, entry->data.to_string());
        entry->Release();
    }

    delete storage;
    delete configuration_manager;
}

TEST_F(LogStorageTest, append_read_badcase) {
    ::system("rm -rf data");
    braft::LogStorage* storage = new braft::SegmentLogStorage("./data");
    braft::ConfigurationManager* configuration_manager = new braft::ConfigurationManager;
    ASSERT_EQ(0, storage->init(configuration_manager));

    // append entry
    for (int i = 0; i < 100000; i++) {
        std::vector<braft::LogEntry*> entries;
        for (int j = 0; j < 5; j++) {
            int64_t index = 5*i + j + 1;
            braft::LogEntry* entry = new braft::LogEntry();
            entry->type = braft::ENTRY_TYPE_DATA;
            entry->id.term = 1;
            entry->id.index = index;

            char data_buf[128];
            snprintf(data_buf, sizeof(data_buf), "hello, world: %ld", index);
            entry->data.append(data_buf);
            entries.push_back(entry);
        }

        ASSERT_EQ(5, storage->append_entries(entries));

        for (size_t j = 0; j < entries.size(); j++) {
            delete entries[j];
        }
    }

    // check and read
    ASSERT_EQ(storage->first_log_index(), 1);
    ASSERT_EQ(storage->last_log_index(), 100000*5);

    delete storage;
    delete configuration_manager;

    // make file unwrite
    butil::FileEnumerator dir1(butil::FilePath("./data"), false, 
                              butil::FileEnumerator::FILES 
                              | butil::FileEnumerator::DIRECTORIES);
    for (butil::FilePath sub_path = dir1.Next(); !sub_path.empty(); sub_path = dir1.Next()) {
        butil::File::Info info;
        butil::GetFileInfo(sub_path, &info);
        if (!info.is_directory) {
            chmod(sub_path.value().c_str(), 0444);
        }
    }

    // reinit failed, because load open no permission
    storage = new braft::SegmentLogStorage("./data");
    configuration_manager = new braft::ConfigurationManager;
    ASSERT_NE(0, storage->init(configuration_manager));
    delete storage;
    delete configuration_manager;

    butil::FileEnumerator dir2(butil::FilePath("./data"), false, 
                              butil::FileEnumerator::FILES 
                              | butil::FileEnumerator::DIRECTORIES);
    for (butil::FilePath sub_path = dir2.Next(); !sub_path.empty(); sub_path = dir2.Next()) {
        butil::File::Info info;
        butil::GetFileInfo(sub_path, &info);
        if (!info.is_directory) {
            chmod(sub_path.value().c_str(), 0644);
        }
    }

    // reinit success
    storage = new braft::SegmentLogStorage("./data");
    configuration_manager = new braft::ConfigurationManager;
    ASSERT_EQ(0, storage->init(configuration_manager));

    // make file chaos
    butil::FileEnumerator dir3(butil::FilePath("./data"), false, 
                              butil::FileEnumerator::FILES 
                              | butil::FileEnumerator::DIRECTORIES);
    for (butil::FilePath sub_path = dir3.Next(); !sub_path.empty(); sub_path = dir3.Next()) {
        butil::File::Info info;
        butil::GetFileInfo(sub_path, &info);
        if (!info.is_directory) {
            chmod(sub_path.value().c_str(), 0644);

            int fd = ::open(sub_path.value().c_str(), O_RDWR, 0644);
            int64_t off = rand() % info.size;
            int64_t len = rand() % (info.size - off);
            if (len > 4096) {
                len = 4096;
            }
            char data[4096] = {0};
            ::pwrite(fd, data, len, off);
            ::close(fd);
        }
    }

    // read will fail
    for (int i = 0; i < 100000*5; i++) {
        int64_t index = i + 1;
        braft::LogEntry* entry = storage->get_entry(index);
        if (entry) {
            entry->Release();
        }
    }

    delete storage;
    delete configuration_manager;
}

TEST_F(LogStorageTest, configuration) {
    ::system("rm -rf data");
    braft::SegmentLogStorage* storage = new braft::SegmentLogStorage("./data");
    braft::ConfigurationManager* configuration_manager = new braft::ConfigurationManager;
    ASSERT_EQ(0, storage->init(configuration_manager));

    {
        braft::LogEntry entry;
        entry.type = braft::ENTRY_TYPE_NO_OP;
        entry.id.term = 1;
        entry.id.index = 1;

        ASSERT_EQ(0, storage->append_entry(&entry));
    }

    // add peer
    {
        braft::LogEntry entry;
        entry.type = braft::ENTRY_TYPE_CONFIGURATION;
        entry.id.term = 1;
        entry.id.index = 2;
        entry.peers = new std::vector<braft::PeerId>;
        entry.peers->push_back(braft::PeerId("1.1.1.1:1000:0"));
        entry.peers->push_back(braft::PeerId("1.1.1.1:2000:0"));
        entry.peers->push_back(braft::PeerId("1.1.1.1:3000:0"));
        storage->append_entry(&entry);
    }

    // append entry
    for (int i = 0; i < 100000; i++) {
        std::vector<braft::LogEntry*> entries;
        for (int j = 0; j < 5; j++) {
            int64_t index = 3 + i*5+j;
            braft::LogEntry* entry = new braft::LogEntry();
            entry->type = braft::ENTRY_TYPE_DATA;
            entry->id.term = 1;
            entry->id.index = index;

            char data_buf[128];
            snprintf(data_buf, sizeof(data_buf), "hello, world: %ld", index);
            entry->data.append(data_buf);
            entries.push_back(entry);
        }
        ASSERT_EQ(5, storage->append_entries(entries));

        for (size_t j = 0; j < entries.size(); j++) {
            delete entries[j];
        }
    }

    // remove peer
    {
        int64_t index = 2 + 100000*5 + 1;
        braft::LogEntry entry;
        entry.type = braft::ENTRY_TYPE_CONFIGURATION;
        entry.id.term = 1;
        entry.id.index = index;
        entry.peers = new std::vector<braft::PeerId>;
        entry.peers->push_back(braft::PeerId("1.1.1.1:1000:0"));
        entry.peers->push_back(braft::PeerId("1.1.1.1:2000:0"));
        storage->append_entry(&entry);
    }

    delete storage;

    braft::SegmentLogStorage* storage2 = new braft::SegmentLogStorage("./data");
    ASSERT_EQ(0, storage2->init(configuration_manager));

    braft::ConfigurationEntry pair;
    configuration_manager->get(2 + 100000*5, &pair);
    ASSERT_EQ(2, pair.id.index);
    LOG(NOTICE) << pair.conf;

    configuration_manager->get(2 + 100000*5 + 1, &pair);
    ASSERT_EQ(2+100000*5+1, pair.id.index);
    LOG(NOTICE) << pair.conf;

    storage2->truncate_suffix(400000);
    configuration_manager->get(400000, &pair);
    ASSERT_EQ(2, pair.id.index);

    storage2->truncate_prefix(2);
    configuration_manager->get(400000, &pair);
    ASSERT_EQ(2, pair.id.index);

    delete storage2;
}

butil::atomic<int> g_first_read_index(0); 
butil::atomic<int> g_last_read_index(0);
bool g_stop = false;

void* read_thread_routine(void* arg) {
    braft::SegmentLogStorage* storage = (braft::SegmentLogStorage*)arg;
    while (!g_stop) {
        int a = g_first_read_index.load(butil::memory_order_relaxed);
        int b = g_last_read_index.load(butil::memory_order_relaxed);
        EXPECT_LE(a, b);
        int index = butil::fast_rand_in(a, b);
        braft::LogEntry* entry = storage->get_entry(index);
        if (entry != NULL) {
            std::string expect;
            butil::string_printf(&expect, "hello_%d", index);
            EXPECT_EQ(expect, entry->data.to_string());
            entry->Release();
        } else {
            EXPECT_LT(index, storage->first_log_index()) 
                    << "first_read_index=" << g_first_read_index.load()
                    << " last_read_index=" << g_last_read_index.load()
                    << " a=" << a << " b=" << b;
            g_stop = true;
            return NULL;
        }
    }
    return NULL;
}

void* write_thread_routine(void* arg) {
    braft::SegmentLogStorage* storage = (braft::SegmentLogStorage*)arg;
    // Write operation distribution: 
    //  - 10% truncate_prefix
    //  - 10% truncate_suffix,
    //  - 30% increase last_read_index (which stands for commitment in the real
    // world), 
    //  - 50% append new entry
    int next_log_index = storage->last_log_index() + 1;
    while (!g_stop) {
        const int r = butil::fast_rand_in(0, 9);
        if (r < 1) {  // truncate_prefix
            int truncate_index = butil::fast_rand_in(
                    g_first_read_index.load(butil::memory_order_relaxed), 
                    g_last_read_index.load(butil::memory_order_relaxed));
            EXPECT_EQ(0, storage->truncate_prefix(truncate_index));
            g_first_read_index.store(truncate_index, butil::memory_order_relaxed);
        } else if (r < 2) {  // truncate suffix
            int truncate_index = butil::fast_rand_in(
                    g_last_read_index.load(butil::memory_order_relaxed),
                    next_log_index - 1);
            EXPECT_EQ(0, storage->truncate_suffix(truncate_index));
            next_log_index = truncate_index + 1;
        } else if (r < 5) { // increase last_read_index which cannot be truncate
            int next_read_index = butil::fast_rand_in(
                    g_last_read_index.load(butil::memory_order_relaxed),
                    next_log_index - 1);
            g_last_read_index.store(next_read_index, butil::memory_order_relaxed);
        } else  {  // Append entry
            braft::LogEntry* entry = new braft::LogEntry;
            entry->type = braft::ENTRY_TYPE_DATA;
            entry->id.index = next_log_index;
            std::string data;
            butil::string_printf(&data, "hello_%d", next_log_index);
            entry->data.append(data);
            ++next_log_index;
            EXPECT_EQ(0, storage->append_entry(entry));
            entry->Release();
        }
    }
    return NULL;
}

namespace braft {
DECLARE_int32(raft_max_segment_size);
}

TEST_F(LogStorageTest, multi_read_single_modify_thread_safe) {
    int32_t saved_max_segment_size = braft::FLAGS_raft_max_segment_size;
    braft::FLAGS_raft_max_segment_size = 1024;
    system("rm -rf ./data");
    braft::SegmentLogStorage* storage = new braft::SegmentLogStorage("./data");
    braft::ConfigurationManager* configuration_manager = new braft::ConfigurationManager;
    ASSERT_EQ(0, storage->init(configuration_manager));
    const int N = 10000;
    for (int i = 1; i <= N; ++i) {
        braft::LogEntry* entry = new braft::LogEntry;
        entry->type = braft::ENTRY_TYPE_DATA;
        entry->id.index = i;
        std::string data;
        butil::string_printf(&data, "hello_%d", i);
        entry->data.append(data);
        ASSERT_EQ(0, storage->append_entry(entry));
        entry->Release();
    }
    ASSERT_EQ(N, storage->last_log_index());
    g_stop = false;
    g_first_read_index.store(1);
    g_last_read_index.store(N);
    bthread_t read_thread[8];
    for (size_t i = 0; i < ARRAY_SIZE(read_thread); ++i) {
        ASSERT_EQ(0, bthread_start_urgent(&read_thread[i], NULL, 
                                   read_thread_routine, storage));
    }
    bthread_t write_thread;
    ASSERT_EQ(0, bthread_start_urgent(&write_thread, NULL,
                                      write_thread_routine, storage));
    ::usleep(5 * 1000 * 1000);
    g_stop = true;
    for (size_t i = 0; i < ARRAY_SIZE(read_thread); ++i) {
        bthread_join(read_thread[i], NULL);
    }
    bthread_join(write_thread, NULL);

    delete configuration_manager;
    delete storage;
    braft::FLAGS_raft_max_segment_size = saved_max_segment_size;
}

TEST_F(LogStorageTest, large_entry) {
    system("rm -rf ./data");
    braft::SegmentLogStorage* storage = new braft::SegmentLogStorage("./data");
    braft::ConfigurationManager* configuration_manager = new braft::ConfigurationManager;
    ASSERT_EQ(0, storage->init(configuration_manager));
    braft::LogEntry* entry = new braft::LogEntry;
    entry->type = braft::ENTRY_TYPE_DATA;
    entry->id.index = 1;
    entry->id.term = 1;
    std::string data;
    data.resize(512 * 1024 * 1024, 'a');
    entry->data.append(data);
    ASSERT_EQ(0, storage->append_entry(entry));
    entry->Release();
    entry = storage->get_entry(1);
    ASSERT_EQ(data, entry->data.to_string());
    entry->Release();

    ASSERT_EQ(1, storage->_first_log_index); 
    ASSERT_EQ(1, storage->_last_log_index);
    ASSERT_EQ(0, storage->_segments.size());
    scoped_refptr<braft::Segment> segment = storage->open_segment(); 
    ASSERT_EQ(1, storage->_segments.size());

    braft::SegmentLogStorage* storage2 = new braft::SegmentLogStorage("./data");
    braft::ConfigurationManager* configuration_manager2 = new braft::ConfigurationManager;
    ASSERT_EQ(0, storage2->init(configuration_manager2));
    ASSERT_EQ(1, storage2->_first_log_index); 
    ASSERT_EQ(1, storage2->_last_log_index);
    ASSERT_EQ(1, storage2->_segments.size());
}

TEST_F(LogStorageTest, reboot_with_checksum_type_changed) {
    system("rm -rf ./data");
    braft::SegmentLogStorage* storage = new braft::SegmentLogStorage("./data");
    braft::ConfigurationManager* configuration_manager = new braft::ConfigurationManager;
    ASSERT_EQ(0, storage->init(configuration_manager));
    storage->_checksum_type = 0;  // murmurhash
    for (int i = 0; i < 10; i++) {
        braft::LogEntry* entry = new braft::LogEntry();
        entry->type = braft::ENTRY_TYPE_DATA;
        entry->id.term = 1;
        entry->id.index = i + 1;

        char data_buf[128];
        snprintf(data_buf, sizeof(data_buf), "hello, world: %d", i + 1);
        entry->data.append(data_buf);

        ASSERT_EQ(0, storage->append_entry(entry));

        entry->Release();
    }
    delete storage;
    storage = new braft::SegmentLogStorage("./data");
    ASSERT_EQ(0, storage->init(configuration_manager));
    storage->_checksum_type = 1;  // crc32
    for (int i = 10; i < 20; i++) {
        braft::LogEntry* entry = new braft::LogEntry();
        entry->type = braft::ENTRY_TYPE_DATA;
        entry->id.term = 1;
        entry->id.index = i + 1;

        char data_buf[128];
        snprintf(data_buf, sizeof(data_buf), "hello, world: %d", i + 1);
        entry->data.append(data_buf);

        ASSERT_EQ(0, storage->append_entry(entry));

        entry->Release();
    }
    delete storage;
    storage = new braft::SegmentLogStorage("./data");
    ASSERT_EQ(0, storage->init(configuration_manager));
    for (int index = 1; index <= 20; ++index) {
        braft::LogEntry* entry = storage->get_entry(index);
        ASSERT_EQ(entry->id.term, 1);
        ASSERT_EQ(entry->type, braft::ENTRY_TYPE_DATA);
        ASSERT_EQ(entry->id.index, index);

        char data_buf[128];
        snprintf(data_buf, sizeof(data_buf), "hello, world: %d", index);
        ASSERT_EQ(data_buf, entry->data.to_string());
        entry->Release();
    }
    
    delete storage;
}

TEST_F(LogStorageTest, joint_configuration) {
    system("rm -rf ./data");
    braft::ConfigurationManager cm;
    std::unique_ptr<braft::SegmentLogStorage>
                log_storage(new braft::SegmentLogStorage("./data"));
    ASSERT_EQ(0, log_storage->init(&cm));
    for (int i = 1; i <= 20; ++i) {
        scoped_refptr<braft::LogEntry> entry = new braft::LogEntry;
        entry->id = braft::LogId(i, 1);
        entry->peers = new std::vector<braft::PeerId>;
        entry->type = braft::ENTRY_TYPE_CONFIGURATION;
        for (int j = 0; j < 3; ++j) {
            entry->peers->push_back("127.0.0.1:" + std::to_string(i + j));
        }
        entry->old_peers = new std::vector<braft::PeerId>;
        for (int j = 1; j <= 3; ++j) {
            entry->old_peers->push_back("127.0.0.1:" + std::to_string(i + j));
        }
        ASSERT_EQ(0, log_storage->append_entry(entry));
    }

    for (int i = 1; i <= 20; ++i) {
        braft::LogEntry* entry = log_storage->get_entry(i);
        ASSERT_TRUE(entry != NULL);
        ASSERT_EQ(entry->type, braft::ENTRY_TYPE_CONFIGURATION);
        ASSERT_TRUE(entry->peers != NULL);
        ASSERT_TRUE(entry->old_peers != NULL);
        braft::Configuration conf;
        for (int j = 0; j < 3; ++j) {
            conf.add_peer("127.0.0.1:" + std::to_string(i + j));
        }
        braft::Configuration old_conf;
        for (int j = 1; j <= 3; ++j) {
            old_conf.add_peer("127.0.0.1:" + std::to_string(i + j));
        }
        ASSERT_TRUE(conf.equals(*entry->peers))
            << conf << " xxxx " << braft::Configuration(*entry->peers);
                    
        ASSERT_TRUE(old_conf.equals(*entry->old_peers));
        entry->Release();
    }

    // Restart
    log_storage.reset(new braft::SegmentLogStorage("./data"));
    ASSERT_EQ(0, log_storage->init(&cm));
    for (int i = 1; i <= 20; ++i) {
        braft::LogEntry* entry = log_storage->get_entry(i);
        ASSERT_TRUE(entry != NULL);
        ASSERT_EQ(entry->type, braft::ENTRY_TYPE_CONFIGURATION);
        ASSERT_TRUE(entry->peers != NULL);
        ASSERT_TRUE(entry->old_peers != NULL);
        braft::Configuration conf;
        for (int j = 0; j < 3; ++j) {
            conf.add_peer("127.0.0.1:" + std::to_string(i + j));
        }
        braft::Configuration old_conf;
        for (int j = 1; j <= 3; ++j) {
            old_conf.add_peer("127.0.0.1:" + std::to_string(i + j));
        }
        ASSERT_TRUE(conf.equals(*entry->peers))
            << conf << " xxxx " << braft::Configuration(*entry->peers);
                    
        ASSERT_TRUE(old_conf.equals(*entry->old_peers));
        entry->Release();
    }

    for (int i = 1; i <= 20; ++i) {
        braft::LogEntry* entry = log_storage->get_entry(i);
        ASSERT_TRUE(entry != NULL);
        ASSERT_EQ(entry->type, braft::ENTRY_TYPE_CONFIGURATION);
        ASSERT_TRUE(entry->peers != NULL);
        ASSERT_TRUE(entry->old_peers != NULL);
        ASSERT_EQ(1, entry->id.term);
        braft::Configuration conf;
        for (int j = 0; j < 3; ++j) {
            conf.add_peer("127.0.0.1:" + std::to_string(i + j));
        }
        braft::Configuration old_conf;
        for (int j = 1; j <= 3; ++j) {
            old_conf.add_peer("127.0.0.1:" + std::to_string(i + j));
        }
        ASSERT_TRUE(conf.equals(*entry->peers));
        ASSERT_TRUE(old_conf.equals(*entry->old_peers));
        entry->Release();
    }

    for (int i = 1; i <= 20; ++i) {
        braft::ConfigurationEntry entry;
        cm.get(i, &entry);
        ASSERT_EQ(braft::LogId(i, 1), entry.id);
        braft::Configuration conf;
        for (int j = 0; j < 3; ++j) {
            conf.add_peer("127.0.0.1:" + std::to_string(i + j));
        }
        braft::Configuration old_conf;
        for (int j = 1; j <= 3; ++j) {
            old_conf.add_peer("127.0.0.1:" + std::to_string(i + j));
        }
        ASSERT_TRUE(conf.equals(entry.conf));
        ASSERT_TRUE(old_conf.equals(entry.old_conf));
    }
}

