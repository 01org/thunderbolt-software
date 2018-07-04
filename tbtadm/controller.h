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

#pragma once

#include <iosfwd>
#include <map>

#include <boost/filesystem.hpp>

namespace fs = boost::filesystem;

namespace tbtadm
{

class Controller
{
public:
    static constexpr int UnkownSL = -1;

    Controller(int argc, char* argv[], std::ostream& out, std::ostream& err);
    void run();

private:
    /// Prints all connected devices
    void devices();

    /// Prints all connected peers (hosts)
    void peers();

    /// Prints all connected devices in a tree
    void topology();

    /// Add to tree all devices under a given path
    struct ControllerInTree;
    void createTree(ControllerInTree& controller, const fs::path& path);

    void printTree(std::string& indentation,
                   const std::map<std::string, ControllerInTree>& map);

    void printDetails(bool last,
                      std::string& indentation,
                      const std::vector<std::string>& details);

    /// Goes over all domains and approves all the connected devices
    void approveAll();

    /// Approves the given device and its descendants
    void approveAll(const fs::path& dir);

    /// Approves the given device
    void approve(const fs::path& dir);

    /// Adds to ACL the given device
    void addToACL(const fs::path& dir);

    /// Adds to Boot ACL the given device
    void addToBootACL(const std::string uuid);

    /// Remove from Boot ACL the given device with uuid
    void removeFromBootACL(const std::string uuid);

    /// Prints ACL
    void acl();

    /// Add the given device to ACL
    void add(const fs::path& dir);

    /// Removes the given UUID from ACL
    void remove(std::string uuid);

    /// Clears the ACL
    void removeAll();

    int m_argc;
    char** m_argv;
    std::ostream& m_out;
    std::ostream& m_err;
    int m_sl    = UnkownSL; // FIXME: Consider moving to a local var
    bool m_once = false;
};

} // namespace tbtadm
