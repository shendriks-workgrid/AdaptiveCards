// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
#include "pch.h"

#include "ActionHelpers.h"
#include "AdaptiveRenderArgs.h"
#include "AdaptiveShowCardActionRenderer.h"
#include "LinkButton.h"
#include "AdaptiveHostConfig.h"

namespace AdaptiveCards::Rendering::WinUI3::ActionHelpers
{
    rtxaml::Thickness GetButtonMargin(rtrender::AdaptiveActionsConfig const& actionsConfig)
    {
        rtxaml::Thickness buttonMargin{};
        const uint32_t buttonSpacing = actionsConfig.ButtonSpacing();
        const auto actionsOrientation = actionsConfig.ActionsOrientation();

        if (actionsOrientation == rtrender::ActionsOrientation::Horizontal)
        {
            buttonMargin.Left = buttonMargin.Right = buttonSpacing / 2;
        }
        else
        {
            buttonMargin.Top = buttonMargin.Bottom = buttonSpacing / 2;
        }

        return buttonMargin;
    }

    void SetTooltip(winrt::hstring const& toolTipText, rtxaml::DependencyObject const& tooltipTarget)
    {
        if (!toolTipText.empty())
        {
            rtxaml::Controls::TextBlock tooltipTextBlock;
            tooltipTextBlock.Text(toolTipText);

            rtxaml::Controls::ToolTip toolTip;
            toolTip.Content(tooltipTextBlock);
            rtxaml::Controls::ToolTipService::SetToolTip(tooltipTarget, toolTip);
        }
    }

    void SetAutomationNameAndDescription(rtxaml::DependencyObject const& dependencyObject,
                                         winrt::hstring const& name,
                                         winrt::hstring const& description)
    {
        rtxaml::Automation::AutomationProperties::SetName(dependencyObject, name);
        rtxaml::Automation::AutomationProperties::SetFullDescription(dependencyObject, description);
    }

    template<typename T> auto to_ref(T const& t)
    {
        return winrt::box_value(t).as<winrt::Windows::Foundation::IReference<T>>();
    }

    void ArrangeButtonContent(winrt::hstring const& actionTitle,
                              winrt::hstring const& actionIconUrl,
                              winrt::hstring const& actionTooltip,
                              winrt::hstring const& actionAccessibilityText,
                              rtrender::AdaptiveActionsConfig const& actionsConfig,
                              rtrender::AdaptiveRenderContext const& renderContext,
                              rtom::ContainerStyle containerStyle,
                              rtrender::AdaptiveHostConfig const& hostConfig,
                              bool allActionsHaveIcons,
                              rtxaml::Controls::Button const& button)
    {
        winrt::hstring title{actionTitle};
        winrt::hstring iconUrl{actionIconUrl};
        winrt::hstring tooltip{actionTooltip};
        winrt::hstring accessibilityText{actionAccessibilityText};

        winrt::hstring name{};
        winrt::hstring description{};

        // TODO: is it correct substitution to HString.IsValid()?
        if (!accessibilityText.empty())
        {
            // If we have accessibility text use that as the name and tooltip as the description
            name = accessibilityText;
            description = tooltip;
        }
        // TODO: is it correct substitution to HString.IsValid()?
        else if (!title.empty())
        {
            // If we have a title, use title as the automation name and tooltip as the description
            name = title;
            description = tooltip;
        }
        else
        {
            // If there's no title, use tooltip as the name
            name = tooltip;
        }

        SetAutomationNameAndDescription(button, name, description);
        SetTooltip(tooltip, button);

        // Check if the button has an iconurl
        // TODO: is it correct substitution to HString != nullptr?
        if (!iconUrl.empty())
        {
            // Get icon configs
            rtrender::IconPlacement iconPlacement = actionsConfig.IconPlacement();
            uint32_t iconSize = actionsConfig.IconSize();

            // Define the alignment for the button contents
            rtxaml::Controls::StackPanel buttonContentsStackPanel{};

            // Create image and add it to the button
            // TODO: should we use adaptive image here at all?
            rtom::AdaptiveImage adaptiveImage{};

            adaptiveImage.Url(iconUrl);
            // TODO: no need to box to convert to IRef, right?
            adaptiveImage.HorizontalAlignment(rtom::HAlignment::Center);

            rtxaml::UIElement buttonIcon{nullptr};

            auto childRenderArgs =
                winrt::make<rtrender::implementation::AdaptiveRenderArgs>(containerStyle, buttonContentsStackPanel, nullptr);

            auto elementRenderers = renderContext.ElementRenderers();

            auto elementRenderer = elementRenderers.Get(L"Image");
            if (elementRenderer != nullptr)
            {
                // TODO: no need to cast adaptiveImage to element right?
                buttonIcon = elementRenderer.Render(adaptiveImage, renderContext, childRenderArgs);
                if (buttonIcon == nullptr)
                {
                    XamlHelpers::SetContent(button, title);
                    return;
                }
            }

            // Create title text block
            rtxaml::Controls::TextBlock buttonText{};
            buttonText.Text(title);
            buttonText.TextAlignment(rtxaml::TextAlignment::Center);
            buttonText.VerticalAlignment(rtxaml::VerticalAlignment::Center);

            // Handle different arrangements inside button
            auto buttonIconAsFrameworkElement = buttonIcon.as<rtxaml::FrameworkElement>();

            // Set icon height to iconSize(aspect ratio is automatically maintained)
            buttonIconAsFrameworkElement.Height(iconSize);

            rtxaml::UIElement separator{nullptr};

            if (iconPlacement == rtrender::IconPlacement::AboveTitle && allActionsHaveIcons)
            {
                buttonContentsStackPanel.Orientation(rtxaml::Controls::Orientation::Vertical);
            }
            else
            {
                buttonContentsStackPanel.Orientation(rtxaml::Controls::Orientation::Horizontal);

                // Only add spacing when the icon must be located at the left of the title
                uint32_t spacingSize = GetSpacingSizeFromSpacing(hostConfig, rtom::Spacing::Default);
                // TODO: We're p
                separator = XamlHelpers::CreateSeparator(renderContext, spacingSize, spacingSize, {0}, false);
            }
        }
    }

    // TODO: refactors this
    void HandleActionStyling(rtom::IAdaptiveActionElement const& adaptiveActionElement,
                             rtxaml::FrameworkElement const& buttonFrameworkElement,
                             bool isOverflowActionButton,
                             rtrender::AdaptiveRenderContext const& renderContext)
    {
        winrt::hstring actionSentiment{};
        if (adaptiveActionElement != nullptr)
        {
            actionSentiment = adaptiveActionElement.Style();
        }

        int32_t isSentimentPositive, isSentimentDestructive{}, isSentimentDefault{};

        auto resourceDictionary = renderContext.OverrideStyles();

        auto contextImpl = peek_innards<rtrender::implementation::AdaptiveRenderContext>(renderContext);

        // If we have an overflow style apply it, otherwise we'll fall back on the default button styling
        if (isOverflowActionButton)
        {
            if (const auto style = XamlHelpers::TryGetResourceFromResourceDictionaries<rtxaml::Style>(resourceDictionary, L"Adaptive.Action.Overflow"))
            {
                buttonFrameworkElement.Style(style);
            }
        }

        if (actionSentiment.empty() || actionSentiment == L"default")
        {
            XamlHelpers::SetStyleFromResourceDictionary(renderContext, L"Adaptive.Action", buttonFrameworkElement);
        }
        else if (actionSentiment == L"positive")
        {
            if (const auto style = XamlHelpers::TryGetResourceFromResourceDictionaries<rtxaml::Style>(resourceDictionary, L"Adaptive.Action.Positive"))
            {
                buttonFrameworkElement.Style(style);
            }
            else
            {
                // By default, set the action background color to accent color

                auto actionSentimentDictionary = contextImpl->GetDefaultActionSentimentDictionary();

                if (const auto style =
                        XamlHelpers::TryGetResourceFromResourceDictionaries<rtxaml::Style>(actionSentimentDictionary,
                                                                                           L"PositiveActionDefaultStyle"))
                {
                    buttonFrameworkElement.Style(style);
                }
            }
        }
        else if (actionSentiment == L"destructive")
        {
            if (const auto style = XamlHelpers::TryGetResourceFromResourceDictionaries<rtxaml::Style>(resourceDictionary, L"Adaptive.Action.Destructive"))
            {
                buttonFrameworkElement.Style(style);
            }
            else
            {
                // By default, set the action text color to attention color
                auto actionSentimentDictionary = contextImpl->GetDefaultActionSentimentDictionary();

                if (const auto style =
                        XamlHelpers::TryGetResourceFromResourceDictionaries<rtxaml::Style>(actionSentimentDictionary,
                                                                                           L"DestructiveActionDefaultStyle"))
                {
                    buttonFrameworkElement.Style(style);
                }
            }
        }
        else
        {
            winrt::hstring actionSentimentStyle = L"Adaptive.Action." + actionSentiment;
            XamlHelpers::SetStyleFromResourceDictionary(renderContext, actionSentimentStyle, buttonFrameworkElement);
        }
    }

    void SetMatchingHeight(rtxaml::FrameworkElement const& elementToChange, rtxaml::FrameworkElement const& elementToMatch)
    {
        elementToChange.Height(elementToMatch.ActualHeight());
        elementToChange.Visibility(rtxaml::Visibility::Visible);
    }

    rtxaml::UIElement BuildAction(rtom::IAdaptiveActionElement const& adaptiveActionElement,
                                  rtrender::AdaptiveRenderContext const& renderContext,
                                  rtrender::AdaptiveRenderArgs const& renderArgs,
                                  bool isOverflowActionButton)
    {
        // TODO: Should we PeekInnards on the rtrender:: types and save ourselves some layers of C++/winrt?

        auto hostConfig = renderContext.HostConfig();
        auto actionsConfig = hostConfig.Actions();

        auto button = CreateAppropriateButton(adaptiveActionElement);
        button.Margin(GetButtonMargin(actionsConfig));

        auto actionsOrientation = actionsConfig.ActionsOrientation();
        auto actionAlignment = actionsConfig.ActionAlignment();
        if (actionsOrientation == rtrender::ActionsOrientation::Horizontal)
        {
            button.HorizontalAlignment(rtxaml::HorizontalAlignment::Stretch);
        }
        else
        {
            rtxaml::HorizontalAlignment newAlignment;
            switch (actionAlignment)
            {
            case rtrender::ActionAlignment::Center:
                newAlignment = rtxaml::HorizontalAlignment::Center;
                break;
            case rtrender::ActionAlignment::Left:
                newAlignment = rtxaml::HorizontalAlignment::Left;
                break;
            case rtrender::ActionAlignment::Right:
                newAlignment = rtxaml::HorizontalAlignment::Center;
                break;
            case rtrender::ActionAlignment::Stretch:
                newAlignment = rtxaml::HorizontalAlignment::Stretch;
                break;
            }
            button.HorizontalAlignment(newAlignment);
        }

        auto containerStyle = renderArgs.ContainerStyle();
        bool allowAboveTitleIconPlacement = renderArgs.AllowAboveTitleIconPlacement();

        winrt::hstring title;
        winrt::hstring iconUrl;
        winrt::hstring tooltip;
        winrt::hstring accessibilityText;
        if (isOverflowActionButton)
        {
            auto hostConfigImpl = peek_innards<rtrender::implementation::AdaptiveHostConfig>(hostConfig);
            title = hostConfigImpl->OverflowButtonText();
            accessibilityText = hostConfigImpl->OverflowButtonAccessibilityText();
        }
        else
        {
            title = adaptiveActionElement.Title();
            iconUrl = adaptiveActionElement.IconUrl();
            tooltip = adaptiveActionElement.Tooltip();
        }

        ArrangeButtonContent(title, iconUrl, tooltip, accessibilityText, actionsConfig, renderContext, containerStyle, hostConfig, allowAboveTitleIconPlacement, button);

        auto showCardActionConfig = actionsConfig.ShowCard();
        auto actionMode = showCardActionConfig.ActionMode();

        if (adaptiveActionElement)
        {
            auto actionInvoker = renderContext.ActionInvoker();
            auto clickToken =
                button.Click([adaptiveActionElement, actionInvoker](winrt::Windows::Foundation::IInspectable const& /*sender*/,
                                                                    rtxaml::RoutedEventArgs const& /*args*/) -> void // TODO: can we just use (auto..)?
                             { actionInvoker.SendActionEvent(adaptiveActionElement); });
            button.IsEnabled(adaptiveActionElement.IsEnabled());
        }

        HandleActionStyling(adaptiveActionElement, button, isOverflowActionButton, renderContext);
        return button;
    }

    bool WarnForInlineShowCard(rtrender::AdaptiveRenderContext const& renderContext,
                               rtom::IAdaptiveActionElement const& action,
                               const std::wstring& warning)
    {
        if (action && (action.ActionType() == rtom::ActionType::ShowCard))
        {
            renderContext.AddWarning(rtom::WarningStatusCode::UnsupportedValue, warning);
            return true;
        }
        else
        {
            return false;
        }
    }

    void HandleKeydownForInlineAction(rtxaml::Input::KeyRoutedEventArgs const& args,
                                      rtrender::AdaptiveActionInvoker const& actionInvoker,
                                      rtom::IAdaptiveActionElement const& inlineAction)
    {
        if (args.Key() == winrt::Windows::System::VirtualKey::Enter)
        {
            auto coreWindow = winrt::Windows::UI::Core::CoreWindow::GetForCurrentThread();

            if ((coreWindow.GetKeyState(winrt::Windows::System::VirtualKey::Shift) ==
                 winrt::Windows::UI::Core::CoreVirtualKeyStates::None) &&
                (coreWindow.GetKeyState(winrt::Windows::System::VirtualKey::Control) ==
                 winrt::Windows::UI::Core::CoreVirtualKeyStates::None))
            {
                actionInvoker.SendActionEvent(inlineAction);
                args.Handled(true);
            }
        }
    }

    rtxaml::UIElement HandleInlineAction(rtrender::AdaptiveRenderContext const& renderContext,
                                         rtrender::AdaptiveRenderArgs const& renderArgs,
                                         rtxaml::UIElement const& textInputUIElement,
                                         rtxaml::UIElement const& textBoxParentContainer,
                                         bool isMultilineTextBox,
                                         rtom::IAdaptiveActionElement const& inlineAction)
    {
        rtom::ActionType actionType = inlineAction.ActionType();
        auto hostConfig = renderContext.HostConfig();

        // Inline ShowCards are not supported for inline actions
        if (WarnForInlineShowCard(renderContext, inlineAction, L"Inline ShowCard not supported for InlineAction"))
        {
            // TODO: is this correct?
            return nullptr;
        }
        if ((actionType == rtom::ActionType::Submit) || (actionType == rtom::ActionType::Execute))
        {
            renderContext.LinkSubmitActionToCard(inlineAction, renderArgs);
        }

        // Create a grid to hold the text box and the action button
        rtxaml::Controls::Grid xamlGrid{};
        auto columnDefinitions = xamlGrid.ColumnDefinitions();

        // Create the first column and add the text box to it
        rtxaml::Controls::ColumnDefinition textBoxColumnDefinition{};
        textBoxColumnDefinition.Width({1, rtxaml::GridUnitType::Star});
        columnDefinitions.Append(textBoxColumnDefinition);

        auto textBoxContainerAsFrameworkElement = textBoxParentContainer.as<rtxaml::FrameworkElement>();

        rtxaml::Controls::Grid::SetColumn(textBoxContainerAsFrameworkElement, 0);
        XamlHelpers::AppendXamlElementToPanel(textBoxContainerAsFrameworkElement, xamlGrid);

        // Create a separator column
        rtxaml::Controls::ColumnDefinition separatorColumnDefinition{};
        separatorColumnDefinition.Width({1.0, rtxaml::GridUnitType::Auto});
        columnDefinitions.Append(separatorColumnDefinition);

        uint32_t spacingSize = GetSpacingSizeFromSpacing(hostConfig, rtom::Spacing::Default);

        auto separator = XamlHelpers::CreateSeparator(renderContext, spacingSize, 0, {0}, false);

        auto separatorAsFrameworkElement = separator.as<rtxaml::FrameworkElement>();

        rtxaml::Controls::Grid::SetColumn(separatorAsFrameworkElement, 1);
        XamlHelpers::AppendXamlElementToPanel(separator, xamlGrid);

        // Create a column for the button

        rtxaml::Controls::ColumnDefinition inlineActionColumnDefinition{};
        inlineActionColumnDefinition.Width({0, rtxaml::GridUnitType::Auto});
        columnDefinitions.Append(inlineActionColumnDefinition);

        winrt::hstring iconUrl = inlineAction.IconUrl();

        rtxaml::UIElement actionUIElement{nullptr};

        // TODO: should I check for null here as well? hstring
        if (!iconUrl.empty())
        {
            // TODO: same thing as with rendering Poster/BackgroundImage, should we resort to something else?
            auto elementRenderers = renderContext.ElementRenderers();
            auto imageRenderer = elementRenderers.Get(L"Image");

            rtom::AdaptiveImage adaptiveImage{};

            adaptiveImage.Url(iconUrl);
            actionUIElement = imageRenderer.Render(adaptiveImage, renderContext, renderArgs);
        }
        else
        {
            // If there's no icon, just use the title text. Put it centered in a grid so it is
            // centered relative to the text box.
            winrt::hstring title = inlineAction.Title();
            rtxaml::Controls::TextBlock titleTextBlock{};
            titleTextBlock.Text(title);

            // TOOD: what about text alignment?
            titleTextBlock.VerticalAlignment(rtxaml::VerticalAlignment::Center);

            rtxaml::Controls::Grid titleGrid{};
            XamlHelpers::AppendXamlElementToPanel(titleTextBlock, titleGrid);

            actionUIElement = titleGrid;
        }
        // Make the action the same size as the text box
        textBoxContainerAsFrameworkElement.Loaded(
            [actionUIElement, textBoxContainerAsFrameworkElement](winrt::Windows::Foundation::IInspectable const& /*sender*/,
                                                                  rtxaml::RoutedEventArgs const& /*args*/) -> void
            {
                auto actionFrameworkElement = actionUIElement.as<rtxaml::FrameworkElement>();
                ActionHelpers::SetMatchingHeight(actionFrameworkElement, textBoxContainerAsFrameworkElement);
            });

        // Wrap the action in a button
        auto touchTargetUIElement = WrapInTouchTarget(nullptr,
                                                      actionUIElement,
                                                      inlineAction,
                                                      renderContext,
                                                      false,
                                                      L"Adaptive.Input.Text.InlineAction",
                                                      L"", // TODO: not sure if it's correct to do here
                                                      !iconUrl.empty());

        auto touchTargetFrameworkElement = touchTargetUIElement.as<rtxaml::FrameworkElement>();

        // Align to bottom so the icon stays with the bottom of the text box as it grows in the multiline case
        touchTargetFrameworkElement.VerticalAlignment(rtxaml::VerticalAlignment::Bottom);

        // Add the action to the column
        // TODO: should I use gridStatics?
        rtxaml::Controls::Grid::SetColumn(touchTargetFrameworkElement, 2);
        XamlHelpers::AppendXamlElementToPanel(touchTargetFrameworkElement, xamlGrid);

        // If this isn't a multiline input, enter should invoke the action
        auto actionInvoker = renderContext.ActionInvoker();

        if (!isMultilineTextBox)
        {
            textInputUIElement.KeyDown(
                [actionInvoker, inlineAction](winrt::Windows::Foundation::IInspectable const& /*sender*/,
                                              rtxaml::Input::KeyRoutedEventArgs const& args) -> void
                // TODO: do I need ActionHelpers:: namespace here?
                { ActionHelpers::HandleKeydownForInlineAction(args, actionInvoker, inlineAction); });
        }

        return xamlGrid;
    }

    rtxaml::UIElement WrapInTouchTarget(rtom::IAdaptiveCardElement const& adaptiveCardElement,
                                        rtxaml::UIElement const& elementToWrap,
                                        rtom::IAdaptiveActionElement const& action,
                                        rtrender::AdaptiveRenderContext const& renderContext,
                                        bool fullWidth,
                                        const std::wstring& style,
                                        winrt::hstring const& altText,
                                        bool allowTitleAsTooltip)
    {
        /*  ComPtr<IAdaptiveHostConfig> hostConfig;
          THROW_IF_FAILED(renderContext->get_HostConfig(&hostConfig));*/
        auto hostConfig = renderContext.HostConfig();

        if (ActionHelpers::WarnForInlineShowCard(renderContext, action, L"Inline ShowCard not supported for SelectAction"))
        {
            // Was inline show card, so don't wrap the element and just return
            /*ComPtr<IUIElement> localElementToWrap(elementToWrap);
            localElementToWrap.CopyTo(finalElement);*/
            return elementToWrap;
        }

        /*ComPtr<IButton> button;*/
        auto button = CreateAppropriateButton(action);

        /*ComPtr<IContentControl> buttonAsContentControl;
        THROW_IF_FAILED(button.As(&buttonAsContentControl));
        THROW_IF_FAILED(buttonAsContentControl->put_Content(elementToWrap));*/
        // Do I need to cast here before setting content?
        button.Content(elementToWrap);

        /*ComPtr<IAdaptiveSpacingConfig> spacingConfig;
        THROW_IF_FAILED(hostConfig->get_Spacing(&spacingConfig));*/
        auto spacingConfig = hostConfig.Spacing();

        uint32_t cardPadding = 0;
        if (fullWidth)
        {
            /*THROW_IF_FAILED(spacingConfig->get_Padding(&cardPadding));*/
            cardPadding = spacingConfig.Padding();
        }

        /*ComPtr<IFrameworkElement> buttonAsFrameworkElement;
        THROW_IF_FAILED(button.As(&buttonAsFrameworkElement));

        ComPtr<IControl> buttonAsControl;
        THROW_IF_FAILED(button.As(&buttonAsControl));*/

        // We want the hit target to equally split the vertical space above and below the current item.
        // However, all we know is the spacing of the current item, which only applies to the spacing above.
        // We don't know what the spacing of the NEXT element will be, so we can't calculate the correct spacing
        // below. For now, we'll simply assume the bottom spacing is the same as the top. NOTE: Only apply spacings
        // (padding, margin) for adaptive card elements to avoid adding spacings to card-level selectAction.
        if (adaptiveCardElement != nullptr)
        {
            /* ABI::AdaptiveCards::ObjectModel::WinUI3::Spacing elementSpacing;
             THROW_IF_FAILED(adaptiveCardElement->get_Spacing(&elementSpacing));*/
            auto elementSpacing = adaptiveCardElement.Spacing();
            uint32_t spacingSize = GetSpacingSizeFromSpacing(hostConfig, elementSpacing);

            // THROW_IF_FAILED(GetSpacingSizeFromSpacing(hostConfig.Get(), elementSpacing, &spacingSize));
            double topBottomPadding = spacingSize / 2.0;

            // For button padding, we apply the cardPadding and topBottomPadding (and then we negate these in the margin)
            /*THROW_IF_FAILED(buttonAsControl->put_Padding({(double)cardPadding, topBottomPadding, (double)cardPadding, topBottomPadding}));*/
            button.Padding({(double)cardPadding, topBottomPadding, (double)cardPadding, topBottomPadding});

            double negativeCardMargin = cardPadding * -1.0;
            double negativeTopBottomMargin = topBottomPadding * -1.0;

            /*THROW_IF_FAILED(buttonAsFrameworkElement->put_Margin(
                {negativeCardMargin, negativeTopBottomMargin, negativeCardMargin, negativeTopBottomMargin}));*/
            // Do I need to cast here?
            button.Margin({negativeCardMargin, negativeTopBottomMargin, negativeCardMargin, negativeTopBottomMargin});
        }

        // Style the hit target button
        /* THROW_IF_FAILED(
             XamlHelpers::SetStyleFromResourceDictionary(renderContext, style.c_str(), buttonAsFrameworkElement.Get()));*/
        XamlHelpers::SetStyleFromResourceDictionary(renderContext, {style.c_str()}, button);

        // Determine tooltip, automation name, and automation description
        winrt::hstring tooltip, name, description;
        /*  HString tooltip;
          HString name;
          HString description;*/
        if (action != nullptr)
        {
            // If we have an action, get it's title and tooltip.
            winrt::hstring title = action.Title();
            tooltip = action.Tooltip();
            // THROW_IF_FAILED(action->get_Title(title.GetAddressOf()));
            // THROW_IF_FAILED(action->get_Tooltip(tooltip.GetAddressOf()));
            //
            // Is this correct way to check if title has the string inside? should I check c_str()?
            // Do empty strings pass here? I could check for title.empty()
            if (title.data())
            {
                // If we have a title, use title as the name and tooltip as the description
                name = title;
                description = tooltip;

                if (!tooltip.data() && allowTitleAsTooltip)
                {
                    // If we don't have a tooltip, set the title to the tooltip if we're allowed
                    tooltip = title;
                }
            }
            else
            {
                // If we don't have a title, use the tooltip as the name
                name = tooltip;
            }

            // Disable the select action button if necessary
            button.IsEnabled(action.IsEnabled());
            /* THROW_IF_FAILED(action->get_IsEnabled(&isEnabled));
             THROW_IF_FAILED(buttonAsControl->put_IsEnabled(isEnabled));*/
        }
        // TODO: is it correct? what else should I check for? should I check if it's empty?? to mimick HString.IsValid()?
        else if (altText.data())
        {
            // If we don't have an action but we've been passed altText, use that for name and tooltip
            // name.Set(altText);
            // tooltip.Set(altText);
            name = altText;
            tooltip = altText;
        }

        /* ComPtr<IDependencyObject> buttonAsDependencyObject;
         THROW_IF_FAILED(button.As(&buttonAsDependencyObject));*/
        // SetAutomationNameAndDescription(buttonAsDependencyObject.Get(), name.Get(), description.Get());
        // SetTooltip(to_winrt(tooltip), to_winrt(buttonAsDependencyObject));
        SetAutomationNameAndDescription(button, name, description);
        SetTooltip(tooltip, button);

        // can we do explicit check? or need to call check_pointer()?
        if (action != nullptr)
        {
            WireButtonClickToAction(button, action, renderContext);
        }

        /*THROW_IF_FAILED(button.CopyTo(finalElement));*/
        return button;
    }

    void WireButtonClickToAction(rtxaml::Controls::Button button, rtom::IAdaptiveActionElement action, rtrender::AdaptiveRenderContext renderContext)
    {
        /* ComPtr<IButton> localButton(button);
         ComPtr<IAdaptiveActionInvoker> actionInvoker;
         THROW_IF_FAILED(renderContext->get_ActionInvoker(&actionInvoker));
         ComPtr<IAdaptiveActionElement> strongAction(action);*/

        // TODO: is this a valid way to do it?
        auto actionInvoker = renderContext.ActionInvoker();
        /* auto strongAction = *winrt::make_self<rtom::IAdaptiveActionElement>(actionInvoker);*/
        rtom::IAdaptiveActionElement strongAction{action};

        auto eventToken = button.Click(
            [strongAction, actionInvoker](winrt::Windows::Foundation::IInspectable const&, rtxaml::RoutedEventArgs const&)
            { actionInvoker.SendActionEvent(strongAction); });
    }

    rtxaml::UIElement HandleSelectAction(rtom::IAdaptiveCardElement const& adaptiveCardElement,
                                         rtom::IAdaptiveActionElement const& selectAction,
                                         rtrender::AdaptiveRenderContext const& renderContext,
                                         rtxaml::UIElement const& uiElement,
                                         bool supportsInteractivity,
                                         bool fullWidthTouchTarget)
    {
        if (selectAction != nullptr && supportsInteractivity)
        {
            // TODO: Fix all instances of checking c_str of hstring to .empty()
            // TODO: Does this pass empty hstring or hstring with null c_str?
            return WrapInTouchTarget(
                adaptiveCardElement, uiElement, selectAction, renderContext, fullWidthTouchTarget, L"Adaptive.SelectAction", {}, true);
        }
        else
        {
            if (selectAction != nullptr)
            {
                renderContext.AddWarning(rtom::WarningStatusCode::InteractivityNotSupported,
                                         {L"SelectAction present, but Interactivity is not supported"});
            }

            return uiElement;
            /*ComPtr<IUIElement> localUiElement(uiElement);
            THROW_IF_FAILED(localUiElement.CopyTo(outUiElement));*/
        }
    }

    void BuildActions(winrt::AdaptiveCards::ObjectModel::WinUI3::AdaptiveCard const& adaptiveCard,
                      winrt::Windows::Foundation::Collections::IVector<winrt::AdaptiveCards::ObjectModel::WinUI3::IAdaptiveActionElement> const& children,
                      winrt::Windows::UI::Xaml::Controls::Panel const& bodyPanel,
                      bool insertSeparator,
                      winrt::AdaptiveCards::Rendering::WinUI3::AdaptiveRenderContext const& renderContext,
                      winrt::AdaptiveCards::Rendering::WinUI3::AdaptiveRenderArgs const& renderArgs)
    {
        auto hostConfig = renderContext.HostConfig();
        auto actionsConfig = hostConfig.Actions();
        if (insertSeparator)
        {
            auto spacingSize = GetSpacingSizeFromSpacing(hostConfig, actionsConfig.Spacing());
            auto separator = XamlHelpers::CreateSeparator(renderContext, spacingSize, 0u, winrt::Windows::UI::Color{}, false);
            XamlHelpers::AppendXamlElementToPanel(separator, bodyPanel);
        }

        auto actionSetControl = BuildActionSetHelper(adaptiveCard, nullptr, children, renderContext, renderArgs);
        XamlHelpers::AppendXamlElementToPanel(actionSetControl, bodyPanel);
    }

    rtxaml::Controls::Button CreateFlyoutButton(rtrender::AdaptiveRenderContext renderContext, rtrender::AdaptiveRenderArgs renderArgs)
    {
        // Create an action button
        /* ComPtr<IUIElement> overflowButtonAsUIElement;
         RETURN_IF_FAILED(BuildAction(nullptr, renderContext, renderArgs, true, &overflowButtonAsUIElement));*/
        auto overflowButtonAsUIElement = BuildAction(nullptr, renderContext, renderArgs, true);

        // Create a menu flyout for the overflow button
        // TODO : is this correct?
        auto overflowButtonAsButtonWithFlyout = overflowButtonAsUIElement.as<rtxaml::Controls::IButtonWithFlyout>();
        /* ComPtr<IButtonWithFlyout> overflowButtonAsButtonWithFlyout;
         RETURN_IF_FAILED(overflowButtonAsUIElement.As(&overflowButtonAsButtonWithFlyout));*/

        /*ComPtr<IMenuFlyout> overflowFlyout =
            XamlHelpers::CreateABIClass<IMenuFlyout>(HStringReference(RuntimeClass_Windows_UI_Xaml_Controls_MenuFlyout));*/

        rtxaml::Controls::MenuFlyout overflowFlyout{};

        /* ComPtr<IFlyoutBase> overflowFlyoutAsFlyoutBase;
         RETURN_IF_FAILED(overflowFlyout.As(&overflowFlyoutAsFlyoutBase));*/
        // RETURN_IF_FAILED(overflowButtonAsButtonWithFlyout->put_Flyout(overflowFlyoutAsFlyoutBase.Get()));
        overflowButtonAsButtonWithFlyout.Flyout(overflowFlyout);

        // Return overflow button
        /*ComPtr<IButton> overFlowButtonAsButton;
        RETURN_IF_FAILED(overflowButtonAsUIElement.As(&overFlowButtonAsButton));
        RETURN_IF_FAILED(overFlowButtonAsButton.CopyTo(overflowButton));*/

        return overflowButtonAsUIElement.as<rtxaml::Controls::Button>();
    }

    rtxaml::UIElement AddOverflowFlyoutItem(rtom::IAdaptiveActionElement const& action,
                                            rtxaml::Controls::Button const& overflowButton,
                                            rtom::AdaptiveCard const& adaptiveCard,
                                            rtom::AdaptiveActionSet const& adaptiveActionSet,
                                            rtxaml::Controls::Panel const& showCardPanel,
                                            rtrender::AdaptiveRenderContext const& renderContext,
                                            rtrender::AdaptiveRenderArgs const& renderArgs)
    {
        // Get the flyout items vector
        auto buttonWithFlyout = overflowButton.as<rtxaml::Controls::IButtonWithFlyout>();
        auto menuFlyout = buttonWithFlyout.Flyout().as<rtxaml::Controls::MenuFlyout>();
        auto flyoutItems = menuFlyout.Items();

        // Create a flyout item
        rtxaml::Controls::MenuFlyoutItem flyoutItem{};

        // Set the title
        flyoutItem.Text(action.Title());

        // Set the tooltip
        SetTooltip(action.Tooltip(), flyoutItem);

        // Hook the menu item up to the action invoker
        rtom::IAdaptiveActionElement actionParam{action};
        rtrender::AdaptiveActionInvoker actionInvoker = renderContext.ActionInvoker();
        // TODO: Do I need this eventToken at all?
        auto eventToken = flyoutItem.Click(
            [actionParam, actionInvoker](winrt::Windows::Foundation::IInspectable const&, rtxaml::RoutedEventArgs const)
            { return actionInvoker.SendActionEvent(actionParam); });

        rtom::ActionType actionType = action.ActionType();
        if (actionType == rtom::ActionType::ShowCard)
        {
            // Build the inline show card.
            BuildInlineShowCard(adaptiveCard, adaptiveActionSet, action, showCardPanel, renderContext, renderArgs);
        }

        // Add the new menu item to the vector
        flyoutItems.Append(flyoutItem);

        // TODO: We don't need to do this, right? WinRT will cast for us?
        return flyoutItem;
    }

    void BuildInlineShowCard(rtom::AdaptiveCard const& adaptiveCard,
                             rtom::AdaptiveActionSet const& adaptiveActionSet,
                             rtom::IAdaptiveActionElement const& action,
                             rtxaml::Controls::Panel const& showCardsPanel,
                             rtrender::AdaptiveRenderContext const& renderContext,
                             rtrender::AdaptiveRenderArgs const& renderArgs)
    {
        auto hostConfig = renderContext.HostConfig();
        auto actionsConfig = hostConfig.Actions();
        auto showCardActionConfig = actionsConfig.ShowCard();
        rtrender::ActionMode showCardActionMode = showCardActionConfig.ActionMode();
        if (showCardActionMode == rtrender::ActionMode::Inline)
        {
            // Get the card to be shown
            auto actionAsShowCardAction = action.as<rtom::AdaptiveShowCardAction>();
            auto showCard = actionAsShowCardAction.Card();

            // Render the card and add it to the show card panel
            auto uiShowCard = winrt::AdaptiveCards::Rendering::WinUI3::implementation::AdaptiveShowCardActionRenderer::BuildShowCard(
                showCard, renderContext, renderArgs, (adaptiveActionSet == nullptr));
            XamlHelpers::AppendXamlElementToPanel(uiShowCard, showCardsPanel);

            // Register the show card with the render context
            auto contextImpl = peek_innards<rtrender::implementation::AdaptiveRenderContext>(renderContext);

            if (adaptiveActionSet)
            {
                contextImpl->AddInlineShowCard(adaptiveActionSet, actionAsShowCardAction, uiShowCard, renderArgs);
            }
            else
            {
                contextImpl->AddInlineShowCard(adaptiveCard, actionAsShowCardAction, uiShowCard, renderArgs);
            }
        }
    }

    rtxaml::UIElement CreateActionButtonInActionSet(
        rtom::AdaptiveCard const& adaptiveCard,
        rtom::AdaptiveActionSet const& adaptiveActionSet,
        rtom::IAdaptiveActionElement const& actionToCreate,
        uint32_t columnIndex,
        rtxaml::Controls::Panel const& actionsPanel,
        rtxaml::Controls::Panel const& showCardPanel,
        winrt::Windows::Foundation::Collections::IVector<rtxaml::Controls::ColumnDefinition> const& columnDefinitions,
        rtrender::AdaptiveRenderContext const& renderContext,
        rtrender::AdaptiveRenderArgs const& renderArgs)
    {
        // Render each action using the registered renderer
        rtom::IAdaptiveActionElement action = actionToCreate;
        auto actionType = action.ActionType();
        auto actionRegistration = renderContext.ActionRenderers();

        rtrender::IAdaptiveActionRenderer renderer;
        while (!renderer)
        {
            auto actionTypeString = action.ActionTypeString();
            renderer = actionRegistration.Get(actionTypeString);
            if (!renderer)
            {
                switch (action.FallbackType())
                {
                case rtom::FallbackType::Drop:
                    XamlHelpers::WarnForFallbackDrop(renderContext, actionTypeString);
                    return nullptr;

                case rtom::FallbackType::Content:
                    action = action.FallbackContent();
                    XamlHelpers::WarnForFallbackContentElement(renderContext, actionTypeString, action.ActionTypeString());
                    break; // Go again

                case rtom::FallbackType::None:
                    throw winrt::hresult_error(E_FAIL); // TODO: Really?
                }
            }
        }

        rtxaml::UIElement actionControl = renderer.Render(action, renderContext, renderArgs);

        XamlHelpers::AppendXamlElementToPanel(actionControl, actionsPanel);

        if (columnDefinitions)
        {
            // If using the equal width columns, we'll add a column and assign the column
            rtxaml::Controls::ColumnDefinition columnDefinition;
            columnDefinition.Width({0, rtxaml::GridUnitType::Auto});
            rtxaml::Controls::Grid::SetColumn(actionControl.as<rtxaml::FrameworkElement>(), columnIndex);
        }

        if (actionType == rtom::ActionType::ShowCard)
        {
            // Build the inline show card.
            BuildInlineShowCard(adaptiveCard, adaptiveActionSet, action, showCardPanel, renderContext, renderArgs);
        }

        return actionControl;
    }

    rtxaml::UIElement BuildActionSetHelper(rtom::AdaptiveCard const& adaptiveCard,
                                           rtom::AdaptiveActionSet const& adaptiveActionSet,
                                           winrt::Windows::Foundation::Collections::IVector<rtom::IAdaptiveActionElement> const& children,
                                           rtrender::AdaptiveRenderContext const& renderContext,
                                           rtrender::AdaptiveRenderArgs const& renderArgs)
    {
        auto hostConfig = renderContext.HostConfig();
        auto actionsConfig = hostConfig.Actions();

        rtrender::ActionAlignment actionAlignment = actionsConfig.ActionAlignment();
        rtrender::ActionsOrientation actionsOrientation = actionsConfig.ActionsOrientation();

        // Declare the panel that will host the buttons
        rtxaml::Controls::Panel actionsPanel{nullptr};
        winrt::Windows::Foundation::Collections::IVector<rtxaml::Controls::ColumnDefinition> columnDefinitions;

        if (actionAlignment == rtrender::ActionAlignment::Stretch && actionsOrientation == rtrender::ActionsOrientation::Horizontal)
        {
            // If stretch alignment and orientation is horizontal, we use a grid with equal column widths to achieve
            // stretch behavior. For vertical orientation, we'll still just use a stack panel since the concept of
            // stretching buttons height isn't really valid, especially when the height of cards are typically dynamic.
            rtxaml::Controls::Grid actionsGrid;
            columnDefinitions = actionsGrid.ColumnDefinitions();
            actionsPanel = actionsGrid.as<rtxaml::Controls::Panel>();
        }
        else
        {
            // Create a stack panel for the action buttons
            rtxaml::Controls::StackPanel actionStackPanel{};

            auto uiOrientation = actionsOrientation == rtrender::ActionsOrientation::Horizontal ?
                rtxaml::Controls::Orientation::Horizontal :
                rtxaml::Controls::Orientation::Vertical;

            actionStackPanel.Orientation(uiOrientation);

            switch (actionAlignment)
            {
            case rtrender::ActionAlignment::Center:
                // TODO: DO I need this cast?
                actionStackPanel.HorizontalAlignment(rtxaml::HorizontalAlignment::Center);
                break;
            case rtrender::ActionAlignment::Left:
                actionStackPanel.HorizontalAlignment(rtxaml::HorizontalAlignment::Left);
                break;
            case rtrender::ActionAlignment::Right:
                actionStackPanel.HorizontalAlignment(rtxaml::HorizontalAlignment::Right);
                break;
            case rtrender::ActionAlignment::Stretch:
                actionStackPanel.HorizontalAlignment(rtxaml::HorizontalAlignment::Stretch);
                break;
            }

            // Add the action buttons to the stack panel
            actionsPanel = actionStackPanel.as<rtxaml::Controls::Panel>();
        }

        auto buttonMargin = GetButtonMargin(actionsConfig);
        if (actionsOrientation == rtrender::ActionsOrientation::Horizontal)
        {
            // Negate the spacing on the sides so the left and right buttons are flush on the side.
            // We do NOT remove the margin from the individual button itself, since that would cause
            // the equal columns stretch behavior to not have equal columns (since the first and last
            // button would be narrower without the same margins as its peers).
            actionsPanel.Margin({buttonMargin.Left * -1, 0, buttonMargin.Right * -1, 0});
        }
        else
        {
            // Negate the spacing on the top and bottom so the first and last buttons don't have extra padding
            actionsPanel.Margin({0, buttonMargin.Top * -1, 0, buttonMargin.Bottom * -1});
        }

        // Get the max number of actions and check the host config to confirm whether we render actions beyond the max in the overflow menu

        auto maxActions = actionsConfig.MaxActions();

        auto hostConfigImpl = peek_innards<rtrender::implementation::AdaptiveHostConfig>(hostConfig);
        bool overflowMaxActions = hostConfigImpl->OverflowMaxActions();

        bool allActionsHaveIcons{true};

        for (auto child : children)
        {
            winrt::hstring iconUrl = child.IconUrl();

            if (iconUrl.empty())
            {
                allActionsHaveIcons = false;
            }
        }

        renderArgs.AllowAboveTitleIconPlacement(allActionsHaveIcons);

        rtxaml::Controls::StackPanel showCardsStackPanel{};
        rtxaml::Controls::Panel showCardsPanel = showCardsStackPanel.as<rtxaml::Controls::Panel>();

        uint32_t currentButtonIndex = 0;
        rtxaml::Controls::Button overflowButton;

        for (auto child : children)
        {
            // TODO: is this correct?
            rtom::IAdaptiveActionElement action{child};
            rtom::ActionMode mode = action.Mode();

            rtom::ActionType actionType = action.ActionType();

            rtxaml::UIElement actionControl{nullptr};

            if (currentButtonIndex < maxActions && mode == rtom::ActionMode::Primary)
            {
                // If we have fewer than the maximum number of actions and this action's mode is primary, make a button
                actionControl = CreateActionButtonInActionSet(
                    adaptiveCard, adaptiveActionSet, action, currentButtonIndex, actionsPanel, showCardsPanel, columnDefinitions, renderContext, renderArgs);

                currentButtonIndex++;
            }
            else if (currentButtonIndex >= maxActions && (mode == rtom::ActionMode::Primary) && !overflowMaxActions)
            {
                // If we have more primary actions than the max actions and we're not allowed to overflow them just set a warning and continue
                renderContext.AddWarning(rtom::WarningStatusCode::MaxActionsExceeded,
                                         {L"Some actions were not rendered due to exceeding the maximum number of actions allowed"});
                return S_OK;
            }
            else
            {
                // If the action's mode is secondary or we're overflowing max actions, create a flyout item on the overflow menu
                if (overflowButton == nullptr)
                {
                    // Create a button for the overflow menu if it doesn't exist yet
                    overflowButton = CreateFlyoutButton(renderContext, renderArgs);
                }

                // Add a flyout item to the overflow menu
                AddOverflowFlyoutItem(action, overflowButton, adaptiveCard, adaptiveActionSet, showCardsPanel, renderContext, renderArgs);

                // If this was supposed to be a primary action but it got overflowed due to max actions, add a warning
                if (mode == rtom::ActionMode::Primary)
                {
                    renderContext.AddWarning(rtom::WarningStatusCode::MaxActionsExceeded,
                                             {L"Some actions were moved to an overflow menu due to exceeding the maximum number of actions allowed"});
                }
            }
        }

        // Lastly add the overflow button itself to the action panel
        if (overflowButton != nullptr)
        {
            // If using equal width columns, add another column and assign the it to the overflow button
            if (columnDefinitions != nullptr)
            {
                rtxaml::Controls::ColumnDefinition columnDefinition{};
                columnDefinition.Width({1.0, rtxaml::GridUnitType::Star});
                columnDefinitions.Append(columnDefinition);

                // TODO: No need to convert, right?
                rtxaml::Controls::Grid::SetColumn(overflowButton, currentButtonIndex);
            }

            // Add the overflow button to the panel
            XamlHelpers::AppendXamlElementToPanel(overflowButton, actionsPanel);

            // Register the overflow button with the render context

            auto contextImpl = peek_innards<rtrender::implementation::AdaptiveRenderContext>(renderContext);

            if (adaptiveActionSet != nullptr)
            {
                // TODO: no need to .as<rtxaml::UIElement> for overflowButton, correct?
                contextImpl->AddOverflowButton(adaptiveActionSet, overflowButton);
            }
            else
            {
                contextImpl->AddOverflowButton(adaptiveCard, overflowButton);
            }
        }

        // Reset icon placement value
        renderArgs.AllowAboveTitleIconPlacement(false);
        XamlHelpers::SetStyleFromResourceDictionary(renderContext, {L"Adapative.Actions"}, actionsPanel);

        rtxaml::Controls::StackPanel actionPanel;

        // Add buttons and show cards to panel
        XamlHelpers::AppendXamlElementToPanel(actionsPanel, actionPanel);
        XamlHelpers::AppendXamlElementToPanel(showCardsStackPanel, actionPanel);

        return actionPanel;
    }

    rtxaml::Controls::Button CreateAppropriateButton(rtom::IAdaptiveActionElement const& action)
    {
        if (action && (action.ActionType() == rtom::ActionType::OpenUrl))
        {
            return winrt::make<LinkButton>();
        }
        else
        {
            return rtxaml::Controls::Button{};
        }
    }
}