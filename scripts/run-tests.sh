set -e

source_dir=$(dirname "$(dirname "$(realpath "$BASH_SOURCE")")")
testapp_dir="testapp"
runner_dir="/tmp/testrunner"
workspace="${runner_dir}/workspace"

export ZMK_BUILD_DIR="$(mktemp -p ${runner_dir} -d build.XXXXXXXXXX)"
export ZMK_SRC_DIR="./zmk/app"
export ZMK_EXTRA_MODULES="${source_dir}"
export ZMK_TESTS_VERBOSE="1"

rm -rf "${workspace}/${testapp_dir}/tests/*"
mkdir -p "${workspace}/${testapp_dir}/tests"
cp -R ${source_dir}/tests/scenarios/* "${workspace}/${testapp_dir}/tests/"

(cd "${workspace}" && ./zmk/app/run-test.sh "${testapp_dir}/tests")
