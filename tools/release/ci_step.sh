#!/bin/sh
# Deva's Awesome Adventures - one step of the CI (.github/workflows/ci.yml) (1.2):
#   tools/release/ci_step.sh "what" command [arguments...]
#
# Runs the command with its output in a file, then prints it. When the command fails, the end of the
# output also becomes error annotations ("::error"), which the checks API of GitHub shows without the
# raw log: who looks into a failure from a script or a phone reads why at once.
what=$1
shift
log=$(mktemp)
"$@" >"$log" 2>&1
rc=$?
cat "$log"
if [ "$rc" -ne 0 ]; then
    tail -n 80 "$log" | awk -v what="$what" -v rc="$rc" '
        { gsub(/\r/, ""); gsub(/%/, "%25"); buf = buf $0 "%0A"; n++ }
        n == 20 { printf "::error title=%s (exit %s)::%s\n", what, rc, buf; buf = ""; n = 0 }
        END { if (n) printf "::error title=%s (exit %s)::%s\n", what, rc, buf }'
fi
rm -f "$log"
exit "$rc"
