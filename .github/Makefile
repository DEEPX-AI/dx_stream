# sudo authorization needed

####################
# [DRI] DevOps
WORKDIR := /root/workdir
####################

####################
# [DRI] ATD
.PHONY: install_dep build load_test download_models install_test_plugin element_unit_test pipeline_test usermeta_test pydxs_test coverage_test unit_test

# install dependency with dx_stream/extern
install_dep: 
	@echo ">> install dependency"
	cd .. && ./install.sh

# build dx_stream
build: 
	@echo ">> start build"
	cd .. && ./build.sh

load_test:
	@echo "===== Start DX-Stream Plugin Load Test ====="
	@TEST_OUTPUT=$$(gst-inspect-1.0 "dxstream" 2>&1)
ifeq ($(shell echo "$$TEST_OUTPUT" | grep -q "No such element or plugin 'dxstream'"; echo $$?),0)
	@echo "[ERROR] Failed to load plugin 'dxstream'!"
	@echo "Error Details:"
	@echo "$$TEST_OUTPUT"
	@exit 1
else
	@echo "[SUCCESS] Plugin 'dxstream' loaded successfully!"
	@exit 0
endif

download_models:
	@echo ">> download models "
	cd .. && ./setup.sh --force

download_models_in_container:
	@echo ">> download models in docker container "
	cd .. && ./setup.sh --force --docker_volume_path="../"

install_test_plugin:
	@echo ">> install_test_plugin is deprecated and no longer required (kept for backward compatibility)"
	@exit 0

element_unit_test:
	@echo "===== Running DX-Stream Unit Tests ====="
	@cd ../test && bash run_element.sh; \
	if [ $$? -eq 0 ]; then \
		echo "[SUCCESS] All tests passed!"; \
	else \
		echo "[ERROR] Tests failed! Exiting with error."; \
		exit 1; \
	fi

pipeline_test:
	@echo "===== Running DX-Stream Pipeline Tests ====="
	@cd ../test && bash run_pipeline.sh; \
	if [ $$? -eq 0 ]; then \
		echo "[SUCCESS] All tests passed!"; \
	else \
		echo "[ERROR] Tests failed! Exiting with error."; \
		exit 1; \
	fi

usermeta_test:
	@echo "===== Running DX-Stream Metadata Tests ====="
	@cd ../test && bash run_metadata.sh; \
	if [ $$? -eq 0 ]; then \
		echo "[SUCCESS] All tests passed!"; \
	else \
		echo "[ERROR] Tests failed! Exiting with error."; \
		exit 1; \
	fi

pydxs_test:
	@echo "===== Running DX-Stream Python Tests ====="
	@cd ../test && bash run_pydxs.sh; \
	if [ $$? -eq 0 ]; then \
		echo "[SUCCESS] All tests passed!"; \
	else \
		echo "[ERROR] Tests failed! Exiting with error."; \
		exit 1; \
	fi

coverage_test:
	@echo "===== Running DX-Stream Coverage Tests ====="
	@cd ../test && bash run_coverage.sh; \
	if [ $$? -eq 0 ]; then \
		echo "[SUCCESS] Coverage tests completed!"; \
	else \
		echo "[ERROR] Coverage tests failed! Exiting with error."; \
		exit 1; \
	fi

unit_test:
	@echo "========================================="
	@echo "  DX-STREAM COMPREHENSIVE UNIT TESTS"
	@echo "========================================="
	@echo ""
	@cd ../test && bash run_all.sh; \
	if [ $$? -eq 0 ]; then \
		echo ""; \
		echo "========================================="; \
		echo "  ALL UNIT TESTS PASSED SUCCESSFULLY!"; \
		echo "========================================="; \
	else \
		echo ""; \
		echo "[ERROR] Tests failed! See above for details."; \
		exit 1; \
	fi

####################
