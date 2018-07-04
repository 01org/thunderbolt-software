/*******************************************************************************
 * Thunderbolt(TM) tbtadm tool
 * This code is distributed under the following BSD-style license:
 *
 * Copyright(c) 2017 Intel Corporation.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright notice,
 *       this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of Intel Corporation nor the names of its contributors
 *       may be used to endorse or promote products derived from this software
 *       without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 ******************************************************************************/

#include "controller.h"

#include <iostream>
#include <sstream>
#include <string>
#include <queue>
#include <random>
#include <iterator>
#include <algorithm>

#include "file.h"

using namespace std::string_literals;

namespace
{
const fs::path acltree          = "/var/lib/thunderbolt/acl";
const fs::path sysfsDevicesPath = "/sys/bus/thunderbolt/devices";

const std::string uniqueIDFilename   = "unique_id";
const std::string authorizedFilename = "authorized";
const std::string vendorFilename     = "vendor_name";
const std::string deviceFilename     = "device_name";
const std::string keyFilename        = "key";
const std::string securityFilename   = "security";

const std::string domain          = "domain";
const std::string boot_acl        = "boot_acl";
const std::string hostRouteString = "-0";
const std::string domainDevtype   = "DEVTYPE=thunderbolt_domain";
const std::string deviceDevtype   = "DEVTYPE=thunderbolt_device";
const std::string xdomainDevtype  = "DEVTYPE=thunderbolt_xdomain";

const std::string opt_devices     = "devices";
const std::string opt_peers       = "peers";
const std::string opt_topology    = "topology";
const std::string opt_approve     = "approve";
const std::string opt_approve_all = "approve-all";
const std::string opt_acl         = "acl";
const std::string opt_add         = "add";
const std::string opt_remove      = "remove";
const std::string opt_remove_all  = "remove-all";
const std::string opt_once_flag   = "--once";

const std::string indent     = "│   ";
const size_t indentLength    = 4;
const std::string indentLast = "    ";

enum security_level
{
    SECURITY_LEVEL_NONE = 0,
    SECURITY_LEVEL_USER,
    SECURITY_LEVEL_SECURE,
    SECURITY_LEVEL_DPONLY,
};

const std::string SYMBOL_PIPE = "│";
const std::string SYMBOL_L    = "└─ ";
const std::string SYMBOL_PLUS = "├─ ";

const std::string green  = "\x1b[0;32m";
const std::string yellow = "\x1b[0;33m";
const std::string normal = "\x1b[0m";

class Highlight
{
public:
    Highlight(std::ostream& out, const std::string& color)
        : m_out(out), m_useColor(::isatty(STDOUT_FILENO))
    {
        if (m_useColor)
            m_out << color;
    }

    ~Highlight()
    {
        if (m_useColor)
            m_out << normal;
    }

private:
    std::ostream& m_out;
    bool m_useColor;
};

std::string read(const fs::path& path)
{
    tbtadm::File file(path, tbtadm::File::Mode::Read);
    auto content = file.read();
    return content;
}

/* Trim right characters */
std::string rtrim(const std::string& str, const std::string& chars = " \n\r")
{
    return str.substr(0, str.find_last_not_of(chars) + 1);
}

std::string readAndTrim(const fs::path& path)
{
    return rtrim(read(path));
}

/**
 * Return the content of the file from given path or "Unknown" + type if empty
 *
 * @param path  path to file to read from
 * @param type  string to return if file is empty, prefixed with "Unknown"
 */
std::string readName(const fs::path& path, const std::string& type)
{
    try
    {
        auto res = readAndTrim(path);
        if (!res.empty())
        {
            return res;
        }
    }
    catch (std::runtime_error&)
    {
        // assuming this is from an empty file
    }
    return "Unknown " + type;
}

std::string readVendor(const fs::path& path)
{
    return readName(path, "vendor");
}

std::string readDevice(const fs::path& path)
{
    return readName(path, "device");
}

bool findUeventAttr(const fs::path& path, const std::string& attribute)
{
    const auto ueventFile = path / "uevent";
    if (!fs::exists(ueventFile))
    {
        return false;
    }
    try
    {
        const auto uevent = read(ueventFile);
        return uevent.find(attribute) != uevent.npos;
    }
    // assuming this is from an empty uevent file
    catch (std::runtime_error&)
    {
        return false;
    }
}

bool isDomain(const fs::path& path)
{
    return findUeventAttr(path, domainDevtype);
}

bool isRouteString(const std::string& str)
{
    return str.size() > 1 && str[1] == '-' && str.find('.') == str.npos;
}

bool isHost(const std::string& str)
{
    return str.size() == 3 && str.substr(1) == hostRouteString;
}

bool isDevice(const fs::path& path)
{
    return findUeventAttr(path, deviceDevtype)
           && !isHost(path.filename().string());
}

bool isXDomain(const fs::path& path)
{
    return findUeventAttr(path, xdomainDevtype);
}

struct SLDetails
{
    int num;
    std::string desc;
};

const std::map<std::string, SLDetails> slMap{{"none", {0, "SL0 (none)"}},
                                             {"user", {1, "SL1 (user)"}},
                                             {"secure", {2, "SL2 (secure)"}},
                                             {"dponly", {3, "SL3 (dponly)"}}};

int findSL()
{
    if (fs::exists(sysfsDevicesPath))
    {
        for (auto& dir : fs::directory_iterator(sysfsDevicesPath))
        {
            if (!is_directory(dir))
            {
                continue;
            }
            if (isDomain(dir.path()))
            {
                return slMap.find(readAndTrim(dir.path() / securityFilename))
                    ->second.num;
            }
        }
    }

    return tbtadm::Controller::UnkownSL;
}

bool sysfsDeviceExists()
{
    if (!fs::exists(sysfsDevicesPath))
    {
        std::cerr << "no thunderbolt devices found\n";
        return false;
    }

    return true;
}
} // namespace

tbtadm::Controller::Controller(int argc,
                               char* argv[],
                               std::ostream& out,
                               std::ostream& err)
    : m_argc(argc), m_argv(argv), m_out(out), m_err(err)
{
}

void tbtadm::Controller::run()
{
    if (m_argc >= 2)
    {
        if (m_argv[1] == opt_devices)
        {
            return devices();
        }
        if (m_argv[1] == opt_peers)
        {
            return peers();
        }
        if (m_argv[1] == opt_topology)
        {
            return topology();
        }
        if (m_argv[1] == opt_approve)
        {
            if (m_argc == 3 || m_argc == 4)
            {
                if (m_argv[2] == opt_once_flag)
                {
                    m_once = true;
                }
                m_sl = findSL();
                return approve(sysfsDevicesPath / m_argv[m_argc - 1]);
            }
        }
        if (m_argv[1] == opt_approve_all)
        {
            if (m_argc == 3 && m_argv[2] == opt_once_flag)
            {
                m_once = true;
            }
            return approveAll();
        }
        if (m_argv[1] == opt_acl)
        {
            return acl();
        }
        if (m_argv[1] == opt_add)
        {
            if (m_argc == 3)
            {
                m_sl = findSL();
                return add(sysfsDevicesPath / m_argv[2]);
            }
        }
        if (m_argv[1] == opt_remove)
        {
            if (m_argc == 3)
            {
                return remove(m_argv[2]);
            }
        }
        if (m_argv[1] == opt_remove_all)
        {
            return removeAll();
        }
    }

    // TODO: help
    const std::string sep = " | ";
    m_out << "Usage: " << opt_devices << sep << opt_peers << sep << opt_topology
          << sep << opt_approve << " [" << opt_once_flag << "] <route-string>"
          << sep << opt_approve_all << " [" << opt_once_flag << ']' << sep
          << opt_acl << sep << opt_add << " <route-string>" << sep << opt_remove
          << " <uuid>|<route-string>" << sep << opt_remove_all << "\n";
    throw std::runtime_error("Wrong usage");
}

void tbtadm::Controller::devices()
{
    if (!sysfsDeviceExists())
    {
        return;
    }

    m_sl = findSL();

    // Find and print devices
    for (auto& dir : fs::directory_iterator(sysfsDevicesPath))
    {
        if (!is_directory(dir))
        {
            continue;
        }
        if (!isDevice(dir.path()))
        {
            continue;
        }

        bool authorized = stoi(readAndTrim(dir.path() / authorizedFilename));

        auto inACL = [sl = m_sl] (const auto& dir)
        {
            auto aclDir = acltree / readAndTrim(dir.path() / uniqueIDFilename);

            if (!fs::exists(aclDir))
            {
                return "not in ACL";
            }
            if (sl == 2 && !fs::exists(aclDir / keyFilename))
            {
                return "not in ACL (no key)";
            }
            return "in ACL";
        };

        // TODO: better formatting
        const auto routeString = dir.path().filename().string();

        Highlight highlight(m_out, authorized ? green : normal);

        m_out << routeString << '\t' << readVendor(dir.path() / vendorFilename)
              << '\t' << readDevice(dir.path() / deviceFilename)
              << '\t' << (authorized ? "authorized" : "non-authorized")
              << '\t' << inACL(dir) << '\n';
    }
}

void tbtadm::Controller::peers()
{
    if (!sysfsDeviceExists())
    {
        return;
    }

    for (auto& dir : fs::directory_iterator(sysfsDevicesPath))
    {
        if (!is_directory(dir))
        {
            continue;
        }
        if (!isXDomain(dir.path()))
        {
            continue;
        }

        chdir(dir.path());

        // TODO: better formatting
        const auto routeString = dir.path().filename().string();

        m_out << routeString << '\t' << readVendor(vendorFilename) << '\t'
              << readDevice(deviceFilename) << std::endl;
    }
}

struct tbtadm::Controller::ControllerInTree
{
    ControllerInTree(std::vector<std::string>&& desc) : m_desc(std::move(desc))
    {
    }
    std::vector<std::string> m_desc;
    std::map<std::string, ControllerInTree> m_children;
};

void tbtadm::Controller::topology()
{
    std::map<int, ControllerInTree> controllers;

    if (!sysfsDeviceExists())
    {
        return;
    }

    for (auto& dir : fs::directory_iterator(sysfsDevicesPath))
    {
        if (!is_directory(dir))
        {
            continue;
        }
        auto p           = dir.path();
        auto routeString = p.filename().string();
        if (isHost(routeString))
        {
            auto num      = routeString[0];
            auto security = p.parent_path() / (domain + num) / securityFilename;
            m_sl          = slMap.find(readAndTrim(security))->second.num;
            std::vector<std::string> desc;
            desc.emplace_back("Controller "s + num + '\n');
            desc.emplace_back("Name: " + readDevice(p / deviceFilename) + ", "
                              + readVendor(p / vendorFilename)
                              + '\n');
            desc.emplace_back("Security level: "
                              + slMap.find(readAndTrim(security))->second.desc
                              + '\n');
            auto i = controllers.emplace(num, std::move(desc)).first;
            createTree(i->second, dir.path());
        }
    }

    std::string indentation;
    for (const auto& host : controllers)
    {
        auto last = host.first == controllers.rbegin()->first;
        m_out << host.second.m_desc[0];

        indentation = last ? indentLast : indent;

        printDetails(
            host.second.m_children.empty(), indentation, host.second.m_desc);
        printTree(indentation, host.second.m_children);
    }
}

void tbtadm::Controller::createTree(ControllerInTree& controller,
                                    const fs::path& path)
{
    auto authorized = [](const auto& path) -> std::string {
        return stoi(readAndTrim(path / authorizedFilename)) ? "Yes" : "No";
    };
    auto inACL = [sl = m_sl](const auto& path)->std::string
    {
        auto aclDir = acltree / readAndTrim(path / uniqueIDFilename);
        if (!fs::exists(aclDir))
        {
            return "No";
        }
        if (sl == 2 && !fs::exists(aclDir / keyFilename))
        {
            return "No (no key)";
        }
        return "Yes";
    };

    for (auto& dir : fs::directory_iterator(path))
    {
        if (!is_directory(dir))
        {
            continue;
        }
        auto p           = dir.path();
        auto routeString = p.filename().string();
        std::vector<std::string> desc;

        if (isDevice(p))
        {
            desc.emplace_back(readDevice(p / deviceFilename) + ", "
                              + readVendor(p / vendorFilename)
                              + "\n");
            desc.emplace_back("Route-string: " + routeString + "\n");
            desc.emplace_back("Authorized: " + authorized(p) + "\n");
            desc.emplace_back("In ACL: " + inACL(p) + "\n");
            desc.emplace_back("UUID: " + readAndTrim(p / uniqueIDFilename)
                              + "\n");
        }
        else if (isXDomain(p))
        {
            desc.emplace_back(readDevice(p / deviceFilename) + ", "
                              + readVendor(p / vendorFilename)
                              + "\n");
            desc.emplace_back("Route-string: " + routeString + "\n");
            desc.emplace_back("UUID: " + readAndTrim(p / uniqueIDFilename)
                              + "\n");
        }
        else
        {
            continue;
        }

        auto i =
            controller.m_children.emplace(routeString, std::move(desc)).first;
        createTree(i->second, dir.path());
    }
}

void tbtadm::Controller::printTree(
    std::string& indentation,
    const std::map<std::string, ControllerInTree>& map)
{
    for (const auto& device : map)
    {
        auto last = device.first == map.rbegin()->first;
        m_out << indentation << SYMBOL_PIPE << "\n";
        m_out << indentation << (last ? SYMBOL_L : SYMBOL_PLUS)
              << device.second.m_desc[0];
        indentation += last ? indentLast : indent;
        printDetails(device.second.m_children.empty(),
                     indentation,
                     device.second.m_desc);
        printTree(indentation, device.second.m_children);
        indentation.resize(indentation.size() - indentLength);
    }
}

void tbtadm::Controller::printDetails(bool last,
                                      std::string& indentation,
                                      const std::vector<std::string>& details)
{
    m_out << indentation << (last ? SYMBOL_L : SYMBOL_PLUS)
          << "Details:\n";

    indentation += last ? indentLast : indent;

    size_t detailsSize = details.size();
    for (size_t i = 1; i < detailsSize; ++i)
    {
        if (i == detailsSize - 1)
            m_out << indentation << SYMBOL_L << details[i];
        else
            m_out << indentation << SYMBOL_PLUS << details[i];
    }
    indentation.resize(indentation.size() - indent.size());
}

void tbtadm::Controller::approveAll()
{
    if (!sysfsDeviceExists())
    {
        return;
    }

    for (auto& dir : fs::directory_iterator(sysfsDevicesPath))
    {
        if (!is_directory(dir))
        {
            continue;
        }
        if (!isDomain(dir.path()))
        {
            continue;
        }
        m_out << "Found domain " << dir << '\n';
        auto domainNum = dir.path().filename().string().substr(domain.size());
        m_sl =
            slMap.find(readAndTrim(dir.path() / securityFilename))->second.num;
        switch (m_sl)
        {
            case SECURITY_LEVEL_USER:
            case SECURITY_LEVEL_SECURE:
                break;
            case SECURITY_LEVEL_NONE:
            case SECURITY_LEVEL_DPONLY:
                m_out << "Approval not relevant in SL" << m_sl << '\n';
                return;
            default:
                m_out << "Unknown Security level " << m_sl << '\n';
                return;
        }
        approveAll(dir / (domainNum + hostRouteString));
    }
}

void tbtadm::Controller::approveAll(const fs::path& dir)
{
    for (auto& child : fs::directory_iterator(dir))
    {
        if (!is_directory(child))
        {
            continue;
        }
        if (fs::exists(child / authorizedFilename))
        {
            m_out << "Found child " << child << '\n';
            approve(child);
            approveAll(child);
        }
    }
}

// TODO: move to tbtadm-helper
void tbtadm::Controller::approve(const fs::path& dir) try
{
    m_out << "Authorizing " << dir << '\n';

    File authorized(dir / authorizedFilename, File::Mode::Read);
    if (std::stoi(authorized.read()))
    {
        m_out << "Already authorized\n";
        return;
    }

    if (!m_once)
    {
        addToACL(dir);
    }

    std::ostringstream keyStream;
    if (m_sl == SECURITY_LEVEL_SECURE && !m_once)
    {
        std::default_random_engine eng(std::random_device{}());
        std::uniform_int_distribution<> dist(0, 0xF);
        keyStream << std::hex;
        std::generate_n(std::ostream_iterator<int>(keyStream), 64, [&] {
            return dist(eng);
        });

        File key(dir / keyFilename, File::Mode::Write);
        key << keyStream.str();
    }

    authorized = File(dir / authorizedFilename, File::Mode::Write);
    authorized << 1;

    m_out << "Authorized\n";
    if (m_sl == SECURITY_LEVEL_SECURE && !m_once)
    {
        File keyACL(acltree / readAndTrim(dir / uniqueIDFilename) / keyFilename,
                    File::Mode::Write,
                    O_CREAT,
                    S_IRUSR);
        keyACL << keyStream.str();
        m_out << "Key saved in ACL\n";
    }
}
catch (std::system_error& e)
{
    m_err << e.code() << ' ' << e.what() << '\n';
}
catch (std::exception& e)
{
    m_err << "Exception: " << e.what() << '\n';
}
catch (...)
{
    m_err << "Unknown exception\n";
}

static void addToDomainBootACL(const fs::path& domain, const std::string uuid)
{
    auto boot_acl_path = domain / boot_acl;

    if (!fs::exists(boot_acl_path))
    {
        /* Cannot find boot_acl */
        return;
    }

    /* Read boot_acl */
    std::stringstream stream(readAndTrim(boot_acl_path));
    /* Check maximum allowed boot_acl string size */
    if (stream.str().size() > 36 * 16 + 1 * 15)
    {
        std::cerr << "boot_acl contains too much characters\n";
        return;
    }

    /* Queue keeps UUIDs entries for easy processing */
    std::queue<std::string> acl_queue;

    std::string substring;
    while(getline(stream, substring, ','))
    {
        /* this moves also uuid to the end of the queue */
        if (substring.size() == 36 && substring != uuid)
        {
            acl_queue.push(substring);
        }
    }

    /* Add UUID to boot_acl queue */
    if (acl_queue.size() == 16)
    {
        std::cout << "Trim boot_acl list, remove oldest entry\n";
        acl_queue.pop();
    }
    acl_queue.push(uuid);

    std::string out;

    for (int i = 0; i < 16; i++)
    {
        if (!acl_queue.empty())
        {
            out.append(acl_queue.front());
            acl_queue.pop();
        }

        if (i < 15)
        {
            out.append(",");
        }
    }

    std::ofstream boot_acl_file(boot_acl_path.string());
    boot_acl_file << out;
    boot_acl_file.close();
}

void tbtadm::Controller::addToBootACL(const std::string uuid)
{
    /* For every domain in sysfs */
    for (auto& dir : fs::directory_iterator(sysfsDevicesPath))
    {
        if (dir.status().type() != fs::directory_file)
        {
            continue;
        }

        if (!isDomain(dir.path()))
        {
            continue;
        }

        addToDomainBootACL(dir, uuid);
    }
}

static void removeFromDomainBootACL(const fs::path& dm, const std::string uuid)
{
    auto boot_acl_path = dm / boot_acl;
    bool amended = false;

    if (!fs::exists(boot_acl_path))
    {
        /* Cannot find boot_acl */
        return;
    }

    /* Read boot_acl */
    std::stringstream stream(readAndTrim(boot_acl_path));
    /* Check maximum allowed boot_acl string size */
    if (stream.str().size() > 36 * 16 + 1 * 15)
    {
        std::cerr << "boot_acl contains too much characters\n";
        return;
    }

    /* Queue keeps UUIDs entries for easy processing */
    std::queue<std::string> acl_queue;

    std::string substring;
    while(getline(stream, substring, ','))
    {
        acl_queue.push(substring);
    }

    std::string out;
    int i = 0;
    while (!acl_queue.empty())
    {
        auto u = acl_queue.front();
        if (u != uuid) {
            out.append(u);
            acl_queue.pop();
        }
        else
        {
            std::cout << "Removed UUID also from Boot ACL\n";
            acl_queue.pop();
            amended = true;
            continue;
        }

        if (i++ < 15)
        {
            out.append(",");
        }
    }

    if (!amended)
    {
        return;
    }

    while (i++ < 15)
    {
        out.append(",");
    }

    std::ofstream boot_acl_file(boot_acl_path.string());
    boot_acl_file << out;
    boot_acl_file.close();
}

void tbtadm::Controller::removeFromBootACL(const std::string uuid)
{
    /* For every domain in sysfs */
    for (auto& dir : fs::directory_iterator(sysfsDevicesPath))
    {
        if (dir.status().type() != fs::directory_file)
        {
            continue;
        }

        if (!isDomain(dir.path()))
        {
            continue;
        }

        removeFromDomainBootACL(dir, uuid);
    }
}

void tbtadm::Controller::addToACL(const fs::path& dir)
{
    auto acl = acltree / readAndTrim(dir / uniqueIDFilename);
    if (fs::exists(acl))
    {
        m_out << "Already in ACL\n";
        return;
    }

    fs::create_directories(acl);
    fs::copy(dir / vendorFilename, acl / vendorFilename);
    fs::copy(dir / deviceFilename, acl / deviceFilename);

    m_out << "Added to ACL\n";

    addToBootACL(acl.filename().string());
}

void tbtadm::Controller::acl()
{
    if (!fs::exists(acltree) || fs::is_empty(acltree))
    {
        m_out << "ACL is empty\n";
        return;
    }

    // Get UUID of all connected devices
    std::map<std::string, bool> uuids;
    if (fs::exists(sysfsDevicesPath))
    {
        for (auto& dir : fs::directory_iterator(sysfsDevicesPath))
        {
            if (!is_directory(dir))
            {
                continue;
            }
            if (!isDevice(dir.path()))
            {
                continue;
            }
            File authorizedFile(dir.path() / authorizedFilename,
                                File::Mode::Read);
            bool authorized = std::stoi(authorizedFile.read());
            std::string uuid(readAndTrim(dir.path() / uniqueIDFilename));
            uuids.insert(std::make_pair(uuid, authorized));
        }
        m_sl = findSL();
    }

    // Print ACL
    bool doNoKey = false;
    for (auto& dir : fs::directory_iterator(acltree))
    {
        const auto p = dir.path();
        if (m_sl != SECURITY_LEVEL_SECURE || fs::exists(p / keyFilename))
        {
            const auto uuid   = p.filename().string();
            auto entry        = uuids.find(uuid);
            bool connected    = entry != uuids.end();
            std::string color = normal;

            if (connected)
                color = entry->second ? green : yellow;

            Highlight highlight(m_out, color);

            m_out << uuid << '\t' << readVendor(p / vendorFilename) << '\t'
                  << readDevice(p / deviceFilename) << '\t'
                  << (connected ? "connected" : "not connected") << "\n";
        }
        else
        {
            doNoKey = true;
        }
    }
    if (doNoKey)
    {
        m_out << "\nACL entries with no key (not for current security mode):\n";
        for (auto& dir : fs::directory_iterator(acltree))
        {
            const auto p = dir.path();
            if (!fs::exists(p / keyFilename))
            {
                const auto uuid   = p.filename().string();
                auto entry        = uuids.find(uuid);
                bool connected    = entry != uuids.end();
                std::string color = normal;

                if (connected)
                    color = entry->second ? green : yellow;

                Highlight highlight(m_out, color);

                m_out << uuid << '\t' << readVendor(p / vendorFilename) << '\t'
                      << readDevice(p / deviceFilename) << '\t'
                      << (connected ? "connected" : "not connected") << "\n";
            }
        }
    }
}

void tbtadm::Controller::add(const fs::path& dir)
{
    switch (m_sl)
    {
        case SECURITY_LEVEL_SECURE:
            m_out << "Adding to ACL on SL2 must be done together with device "
                    "approval\n";
            return;
        case SECURITY_LEVEL_NONE:
        case SECURITY_LEVEL_DPONLY:
            m_out << "Adding to ACL is not relevant in SL" << m_sl << '\n';
            return;
        case SECURITY_LEVEL_USER:
            break;
        default:
            m_out << "Unknown Security level " << m_sl << '\n';
            return;
    }

    addToACL(dir);
}

// TODO: move to tbtadm-helper
void tbtadm::Controller::remove(std::string uuid)
{
    // Identify route-string argument and replace it with the UUID
    if (isRouteString(uuid))
    {
        uuid = readAndTrim(sysfsDevicesPath / uuid / uniqueIDFilename);
    }

    auto acl = acltree / uuid;
    if (!fs::exists(acl))
    {
        m_out << "ACL entry doesn't exist\n";
    }

    fs::remove_all(acl);
    removeFromBootACL(uuid);
}

// TODO: move to tbtadm-helper
void tbtadm::Controller::removeAll()
{
    if (!fs::exists(acltree) || fs::is_empty(acltree))
    {
        m_out << "ACL is empty\n";
        return;
    }
    auto count =
        std::count_if(fs::directory_iterator(acltree),
                      {},
                      [](const auto& dir) { return fs::is_directory(dir); });
    fs::remove_all(acltree);
    m_out << count << " entries removed\n";
}
