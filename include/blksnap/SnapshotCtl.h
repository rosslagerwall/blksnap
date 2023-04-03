/*
 * Copyright (C) 2022 Veeam Software Group GmbH <https://www.veeam.com/contacts.html>
 *
 * This file is part of libblksnap
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option) any
 * later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Lesser Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
#pragma once
/*
 * The low-level abstraction over ioctl for the blksnap kernel module.
 * Allows to interact with the module with minimal overhead and maximum
 * flexibility. Uses structures that are directly passed to the kernel module.
 */

#include <string>
#include <uuid/uuid.h>
#include <cstring>
#include <vector>

#if 0
#ifndef BLKSNAP_MODIFICATION
/* Allow to use additional IOCTL from module modification */
#    define BLKSNAP_MODIFICATION
/* Allow to get any sector state. Can be used only for debug purpose */
#    define BLKSNAP_DEBUG_SECTOR_STATE
#endif
#endif
#include "Sector.h"
#include <linux/blk-filter.h>
#include <linux/blksnap.h>

namespace blksnap
{
    struct SBlksnapEventLowFreeSpace
    {
        unsigned long long requestedSectors;
    };

    struct SBlksnapEventCorrupted
    {
        unsigned int origDevIdMj;
        unsigned int origDevIdMn;
        int errorCode;
    };

    struct SBlksnapEvent
    {
        unsigned int code;
        long long time;
        union
        {
            SBlksnapEventLowFreeSpace lowFreeSpace;
            SBlksnapEventCorrupted corrupted;
        };
    };

    class CSnapshotId
    {
    public:
        CSnapshotId()
        {
            uuid_clear(m_id);
        };
        CSnapshotId(const uuid_t& id)
        {
            uuid_copy(m_id, id);
        };
        CSnapshotId(const __u8 buf[16])
        {
            memcpy(m_id, buf, sizeof(uuid_t));
        };
        CSnapshotId(const std::string& idStr)
        {
            uuid_parse(idStr.c_str(), m_id);
        };

        void FromString(const std::string& idStr)
        {
            uuid_parse(idStr.c_str(), m_id);
        };
        const uuid_t& Get() const
        {
            return m_id;
        };
        std::string ToString() const
        {
            char idStr[64];

            uuid_unparse(m_id, idStr);

            return std::string(idStr);
        };
    private:
        uuid_t m_id;
    };

    class CSnapshotCtl
    {
    public:
        CSnapshotCtl();
        ~CSnapshotCtl();

        CSnapshotId Create();
        void Destroy(const CSnapshotId& id);
        void Collect(std::vector<CSnapshotId>& ids);
        void AppendDiffStorage(const CSnapshotId& id, const std::string& devicePath,
                               const std::vector<struct blksnap_sectors>& ranges);
        void Take(const CSnapshotId& id);
        bool WaitEvent(const CSnapshotId& id, unsigned int timeoutMs, SBlksnapEvent& ev);

        void Version(struct blksnap_version& version);
#ifdef BLK_SNAP_MODIFICATION
        /* Additional functional */
        bool Modification(struct blksnap_mod& mod);
#    ifdef BLK_SNAP_DEBUG_SECTOR_STATE
        void GetSectorState(struct blksnap_dev image_dev_id, off_t offset, struct blksnap_sector_state& state);
#    endif
#endif
    private:
        int m_fd;
    };

}
