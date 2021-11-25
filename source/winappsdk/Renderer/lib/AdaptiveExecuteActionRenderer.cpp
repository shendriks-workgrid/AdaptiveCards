// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
#include "pch.h"

#include "AdaptiveExecuteActionRenderer.h"
#include "AdaptiveExecuteActionRenderer.g.cpp"
#include "ActionHelpers.h"

using namespace ABI::AdaptiveCards::Rendering::WinUI3;
using namespace ABI::AdaptiveCards::ObjectModel::WinUI3;
using namespace Microsoft::WRL;

namespace winrt::AdaptiveCards::Rendering::WinUI3::implementation
{
    rtxaml::UIElement AdaptiveExecuteActionRenderer::Render(rtom::IAdaptiveActionElement const& action,
                                                            rtrender::AdaptiveRenderContext const& renderContext,
                                                            rtrender::AdaptiveRenderArgs const& renderArgs)
    {
        try
        {
            // TODO: do we need to check for errors here? or catch -> nullptr should be enough
            renderContext.LinkSubmitActionToCard(action, renderArgs);
            return ::AdaptiveCards::Rendering::WinUI3::ActionHelpers::BuildAction(action, renderContext, renderArgs, false);
        }
        catch (winrt::hresult_error const& ex)
        {
            // TODO: restore treat warnings as errrors
            // TODO: what do we do here?
            return nullptr;
        }
    }
}