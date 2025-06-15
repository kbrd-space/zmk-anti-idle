set -e

source_dir=$(dirname "$(dirname "$(realpath "$BASH_SOURCE")")")
testapp_dir="testapp"
runner_dir="/tmp/testrunner"
workspace="${runner_dir}/workspace"

rm -rf ${runner_dir}/build.*
rm -rf ${workspace}/${testapp_dir}/tests/*
