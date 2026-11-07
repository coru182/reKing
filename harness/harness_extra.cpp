// Stand-ins the Athelas 1.8.2 SDK's headless harness is missing (its Windows build gets these from runtime/app/ and
// gpu/backend.cpp, which the harness leaves out). Headless only: no achievements are kept, and every renderer the
// device might ask for is the harness's null backend.
#include <memory>
#include <vector>

#include "app/app.h"
#include "gpu/backend.h"

namespace gpu {
std::unique_ptr<Backend> CreateD3D12Backend() { return CreateBackend(); }
std::unique_ptr<Backend> CreateVulkanBackend() { return CreateBackend(); }
namespace vk {
bool Probe() { return false; }
}  // namespace vk
}  // namespace gpu

namespace ach {
uint32_t VirtueDealt() { return 100; }
uint32_t VirtueTaken() { return 100; }
bool Fate() { return false; }
bool FellowshipOn() { return false; }
void SharedDeeds(std::vector<Def>&) {}
void Defeated() {}
void LevelStarted(const char*) {}
void LevelDone(const char*) {}
bool FateDone(const char*) { return false; }
}  // namespace ach
