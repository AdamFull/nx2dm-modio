#pragma once

namespace nxe {
class ModuleContext;
namespace script {
class Host;
}
}

namespace nxm::modio {

void expose_modio_services(nxe::script::Host &host, nxe::ModuleContext &ctx);

}
