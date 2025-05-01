export GZ_TRANSPORT_IMPLEMENTATION=zenoh
export CTEST_OUTPUT_ON_FAILURE=1
# Capture exit code to avoid exiting early
make test || test_exit_code=$?
echo "Summarize zenoh test results"
python3 /junit_to_md.py test_results/*.xml >> $GITHUB_STEP_SUMMARY || true
if [ $test_exit_code -ne 0 ]; then
  exit $test_exit_code ;
fi

