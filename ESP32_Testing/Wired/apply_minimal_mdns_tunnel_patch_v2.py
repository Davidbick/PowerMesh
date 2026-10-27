#!/usr/bin/env python3
"""Apply the PowerMesh test address policy to CHIP Minimal mDNS.

Use only with the PowerMesh test firmware. The patch skips the custom uaN
interface for mDNS listener sockets (so mDNS is transmitted on the physical
LAN interface), but returns fd00:706d::2 as the Matter hostname's IPv6 address.
IPv4 address records are suppressed for this test endpoint.

A backup is created as <source>.powermesh.bak. This script refuses to overwrite
an existing backup or to patch source that already has this change.
"""
from __future__ import annotations

import argparse
import sys
from pathlib import Path

TAG = "PowerMesh test policy"

WLAN_BLOCK = '''    // skip IEEE 802.15.4 / Thread interfaces — not suitable for mDNS
    if (strncmp(name, "wpan", 4) == 0)
    {
        return false;
    }

    return true;
'''
WLAN_BLOCK_REPLACEMENT = '''    // skip IEEE 802.15.4 / Thread interfaces — not suitable for mDNS
    if (strncmp(name, "wpan", 4) == 0)
    {
        return false;
    }

    // PowerMesh uses uaN as a point-to-point tunnel, not a physical LAN.
    // Keep mDNS listeners/multicast on the physical LAN interface.
    if (strncmp(name, "ua", 2) == 0)
    {
        return false;
    }

    return true;
'''

FUNCTION_ANCHOR = 'bool AllInterfaces::CurrentInterfaceHasAddressOfType(chip::Inet::IPAddressType type)'
TUNNEL_ITERATOR = r'''/*
 * PowerMesh test policy: advertise the custom UART-interface ULA as the
 * Matter hostname's IPv6 address, regardless of which physical interface
 * received the mDNS query. Do not publish an IPv4 A record for this endpoint.
 * Apply only to the PowerMesh test firmware.
 */
class TunnelAddressIterator : public mdns::Minimal::IpAddressIterator
{
public:
    explicit TunnelAddressIterator(chip::Inet::IPAddressType addrType) : mAddrType(addrType) {}

    bool Next(chip::Inet::IPAddress & dest) override
    {
        if (mReturned)
        {
            return false;
        }
        mReturned = true;

#if INET_CONFIG_ENABLE_IPV4
        if (mAddrType == chip::Inet::IPAddressType::kIPv4)
        {
            return false;
        }
#endif
        if (mAddrType != chip::Inet::IPAddressType::kIPv6 && mAddrType != chip::Inet::IPAddressType::kAny)
        {
            return false;
        }

        if (!chip::Inet::IPAddress::FromString("fd00:706d::2", dest))
        {
            ChipLogError(Discovery, "Failed to parse PowerMesh tunnel IPv6 address");
            return false;
        }
        return true;
    }

private:
    const chip::Inet::IPAddressType mAddrType;
    bool mReturned = false;
};

'''

ADDRESS_ITERATOR_RETURN = 'return chip::Platform::UniquePtr<IpAddressIterator>(chip::Platform::New<AllAddressesIterator>(interfaceId, type));'
ADDRESS_ITERATOR_REPLACEMENT = '''(void) interfaceId;
        return chip::Platform::UniquePtr<IpAddressIterator>(chip::Platform::New<TunnelAddressIterator>(type));'''


def replace_once(text: str, old: str, new: str, label: str) -> str:
    n = text.count(old)
    if n != 1:
        raise ValueError(f"Expected exactly one match for {label}, found {n}; no file written")
    return text.replace(old, new, 1)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source_file", type=Path, help="Active CHIP AddressPolicy_DefaultImpl.cpp")
    args = parser.parse_args()

    path = args.source_file
    if not path.exists():
        print(f"ERROR: source file not found: {path}", file=sys.stderr)
        return 2
    # Resolve symlinks to patch the actual source file used by the build.
    path = path.resolve(strict=True)
    if not path.is_file():
        print(f"ERROR: not a file: {path}", file=sys.stderr)
        return 2

    original = path.read_text(encoding="utf-8")
    if TAG in original or "class TunnelAddressIterator" in original:
        print("ERROR: PowerMesh policy appears to be already applied; no file written.", file=sys.stderr)
        return 2

    text = original
    try:
        include_anchor = "#include <lib/support/logging/CHIPLogging.h>"
        text = replace_once(text, include_anchor,
                            include_anchor + "\n\n#include <string.h>",
                            "CHIP logging include")
        text = replace_once(text, WLAN_BLOCK, WLAN_BLOCK_REPLACEMENT,
                            "wpan interface-filter block")
        # Insert immediately before the definition, independently of blank-line style
        # between AllAddressesIterator's closing brace and this function.
        text = replace_once(text, FUNCTION_ANCHOR,
                            TUNNEL_ITERATOR + FUNCTION_ANCHOR,
                            "AllInterfaces::CurrentInterfaceHasAddressOfType definition")
        text = replace_once(text, ADDRESS_ITERATOR_RETURN,
                            ADDRESS_ITERATOR_REPLACEMENT,
                            "GetIpAddressesForEndpoint return")
    except ValueError as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        return 2

    backup = Path(str(path) + ".powermesh.bak")
    if backup.exists():
        print(f"ERROR: refusing to overwrite existing backup: {backup}", file=sys.stderr)
        return 2

    # Backup the original before writing the modified source.
    backup.write_text(original, encoding="utf-8")
    path.write_text(text, encoding="utf-8")
    print(f"Patched source: {path}")
    print(f"Original backup: {backup}")
    print("Policy applied: uaN excluded as mDNS listener; AAAA response address set to fd00:706d::2; A records suppressed.")
    print("This patch is for the PowerMesh test firmware only.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
