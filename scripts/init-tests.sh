set -e

source_dir=$(dirname "$(dirname "$(realpath "$BASH_SOURCE")")")
testapp_dir="testapp"
runner_dir="/tmp/testrunner"
workspace="${runner_dir}/workspace"

mkdir -p "${workspace}/${testapp_dir}"
cp -R ${source_dir}/tests/app/* "${workspace}/${testapp_dir}/"

if [ ! -d "${workspace}/.west" ]; then
    (cd "${workspace}" && west init -l "${testapp_dir}")
    (cd "${workspace}" && west update --fetch-opt=--filter=tree:0)
    (cd "${workspace}" && west zephyr-export)
fi
