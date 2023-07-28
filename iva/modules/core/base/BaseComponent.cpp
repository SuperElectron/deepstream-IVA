#include "BaseComponent.h"

using njson = nlohmann::json;
using namespace core;

BaseComponent::BaseComponent(Mediator *mediator = nullptr) : _mediator(mediator)
{
    LOG(WARNING) << "created new module: " << "[add name]";
}

