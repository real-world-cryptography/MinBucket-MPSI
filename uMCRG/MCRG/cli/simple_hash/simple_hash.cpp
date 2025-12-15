#include <csignal>
#include <fstream>
#include <functional>
#include <iostream>
#include <string>
#include <time.h>
#if defined(__GNUC__) && (__GNUC__ < 8) && !defined(__clang__)
#include <experimental/filesystem>
#else
#include <filesystem>
#endif

// APSU
#include "apsu/log.h"
#include "apsu/oprf/oprf_sender.h"
#include "apsu/thread_pool_mgr.h"
#include "apsu/version.h"
#include "common_utils.h"
#include "csv_reader.h"
#include "receiver/clp.h"
#include "receiver/receiver_utils.h"



#include "apsu/zmq/receiver_dispatcher_ddh.h"
// #include "Kunlun/crypto/setup.hpp"

using namespace std;
#if defined(__GNUC__) && (__GNUC__ < 8) && !defined(__clang__)
namespace fs = std::experimental::filesystem;
#else
namespace fs = std::filesystem;
#endif
using namespace apsu;
using namespace apsu::receiver;
using namespace apsu::network;
using namespace apsu::oprf;



unique_ptr<CSVReader::DBData> load_db(const string &db_file);

void create_receiver_db(
    const CSVReader::DBData &db_data,
    unique_ptr<PSUParams> psu_params,
    size_t nonce_byte_count,
    bool compress,
    int serial_number
    );

void try_load_csv_db(const CLP &cmd);

void sigint_handler(int param [[maybe_unused]]);


int main(int argc, char *argv[])
{

    prepare_console();
    CLP cmd("Example of a Receiver implementation", APSU_VERSION);
    if (!cmd.parse_args(argc, argv)) {
        APSU_LOG_ERROR("Failed parsing command line arguments");
        return -1;
    }
    std::cout<<"serial number: "<<cmd.serial_number()<<std::endl;

    std::string s = std::to_string(cmd.serial_number());
    std::cout<<s<<std::endl;
    ThreadPoolMgr::SetThreadCount(cmd.threads());
    APSU_LOG_INFO("Setting thread count to " << ThreadPoolMgr::GetThreadCount());
    signal(SIGINT, sigint_handler);

    clock_t start,end;
    start=clock();

    throw_if_file_invalid(cmd.db_file());
    try_load_csv_db(cmd);

    end=clock();
    printf("totile time=%f ms\n",(float)(end-start)*1000/CLOCKS_PER_SEC);
    
}

void sigint_handler(int param [[maybe_unused]])
{
    APSU_LOG_WARNING("Receiver interrupted");
    exit(0);
}


void try_load_csv_db(const CLP &cmd)
{
    unique_ptr<PSUParams> params = build_psu_params(cmd);
    if (!params) {
        // We must have valid parameters given
        APSU_LOG_ERROR("Failed to set PSU parameters");
        //return nullptr;
    }

    unique_ptr<CSVReader::DBData> db_data;
    if (cmd.db_file().empty() || !(db_data = load_db(cmd.db_file()))) {
        // Failed to read db file
        APSU_LOG_DEBUG("Failed to load data from a CSV file");
        //return nullptr;
    }

   create_receiver_db( *db_data, move(params),cmd.nonce_byte_count(), cmd.compress(),cmd.serial_number());
}

//read csv file
unique_ptr<CSVReader::DBData> load_db(const string &db_file)
{
    CSVReader::DBData db_data;
    try {
        CSVReader reader(db_file);
        tie(db_data, ignore) = reader.read();
    } catch (const exception &ex) {
        APSU_LOG_WARNING("Could not open or read file `" << db_file << "`: " << ex.what());
        return nullptr;
    }

    return make_unique<CSVReader::DBData>(move(db_data));
}

void create_receiver_db(
    const CSVReader::DBData &db_data,
    unique_ptr<PSUParams> psu_params,
    size_t nonce_byte_count,
    bool compress,
    int serial_number
    )
{
    if (!psu_params) {
        APSU_LOG_ERROR("No PSU parameters were given");
        //return nullptr;
    }

    shared_ptr<ReceiverDB> receiver_db;
    if (holds_alternative<CSVReader::UnlabeledData>(db_data)) {
        try {
            receiver_db = make_shared<ReceiverDB>(*psu_params, 0, 0, compress);
            receiver_db->my_set_data(get<CSVReader::UnlabeledData>(db_data),serial_number);

            APSU_LOG_INFO(
                "Created unlabeled ReceiverDB with " << receiver_db->get_item_count() << " items");
        } catch (const exception &ex) {
            APSU_LOG_ERROR("Failed to create ReceiverDB: " << ex.what());
            //return nullptr;
        }
    }  else {
        // Should never reach this point
        APSU_LOG_ERROR("Loaded database is in an invalid state");
        //return nullptr;
    }

    if (compress) {
        APSU_LOG_INFO("Using in-memory compression to reduce memory footprint");
    }

}