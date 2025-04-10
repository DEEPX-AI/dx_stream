# RELEASE_NOTES

## v1.3.0 / 2025-04-10
### 1. Changed
- None
### 2. Fixed
- add option '--force' in the setup scritps [DSA1-228](https://deepx.atlassian.net/browse/DSA1-288)
- update get_resource.sh to check internel structure of the tar file [DSA1-228](https://deepx.atlassian.net/browse/DSA1-288)
### 3. Added
- None

## v1.2.2 / 2025-03-25
### 1. Changed
- None
### 2. Fixed
- Fix buffer shuffle problem in dx-infer element [DSA1-328](https://deepx.atlassian.net/browse/DSA1-328)
### 3. Added
- None

## v1.2.1 / 2025-03-20
### 1. Changed
- Download assets(model, video) from AWS S3 [DSA1-324](https://deepx.atlassian.net/browse/DSA1-324)
### 2. Fixed
- update dependency installation (librdkafka) [DSA1-323](https://deepx.atlassian.net/browse/DSA1-323)
### 3. Added
- None

## v1.2.0 / 2025-03-10
### 1. Changed
- None
### 2. Fixed
- None
### 3. Added
- Update pipeline scripts for supporting intel GPU HW Acceleration (VAAPI) [CSP-390](https://deepx.atlassian.net/browse/CSP-390)

## v1.1.1 / 2025-03-05
### 1. Changed
- DX-RT API interface changed (inference engine function) [DSA1-254](https://deepx.atlassian.net/browse/DSA1-254)
- setup_dxnn_assets.sh 실행 시, regression ID 번호를 인자로 받아서 해당하는 모델들을 복사해서 가져오도록 변경. (default regID : 3148)
### 2. Fixed
- Memory free issue (NV12 format Resize) [DSA1-254](https://deepx.atlassian.net/browse/DSA1-254)
- display crash issue (NV12 format color converting) [DSA1-254](https://deepx.atlassian.net/browse/DSA1-254)
- DX-RT에서 DataType의 순서가 변경됨에 따라, dxcommon.hpp에서도 동일하게 DataType을 수정.
- PPU 모델의 경우 Output 배열에 Batch Size 차원이 추가됨에 따라, 관련된 PostProcess 코드들도 이에 맞게 수정. (YOLO, SCRFD)
### 3. Added
- None
