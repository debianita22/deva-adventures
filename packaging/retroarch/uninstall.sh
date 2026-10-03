#!/bin/sh
# Deva's Awesome Adventures - removes the core, its .info and its data from RetroArch (the saves and
# the backups stay). The same options as install.sh (--cfg, --cores, --info, --system, --saves,
# --yes, --dry-run); to go back to an earlier version instead: sh install.sh --restore <backup>.
exec sh "$(dirname "$0")/install.sh" --uninstall "$@"
