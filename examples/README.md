# Nois Examples

Some of these examples use the Steinberg VST3 SDK. The SDK is licensed under the Steinberg VST3 Licensing Agreement. See libs/vst3sdk/LICENSE.txt for more details about the VST3 SDK license.

## Building Versio

- Clone the repository with `git clone --recursive git@github.com:bmqn/nois.git`
- Generate the build with `cmake -Bnois-build -DNOIS_EXAMPLES_PLATFORM=Versio nois`
- Build with `cmake --build nois-build`
- Firmware is now at `nois-build/examples/nois-stretch/versio/nois-stretch-versio.hex`