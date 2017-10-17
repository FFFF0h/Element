/*
    ContentComponent.cpp - This file is part of Element
    Copyright (C) 2015-2017  Kushview, LLC.  All rights reserved.
*/

#include "controllers/AppController.h"
#include "engine/GraphProcessor.h"
#include "Commands.h"
#include "gui/AudioIOPanelView.h"
#include "gui/PluginsPanelView.h"
#include "gui/ConnectionGrid.h"
#include "gui/NavigationView.h"
#include "gui/SessionTreePanel.h"
#include "gui/ViewHelpers.h"
#include "gui/LookAndFeel.h"
#include "session/DeviceManager.h"
#include "session/PluginManager.h"
#include "session/Node.h"
#include "session/UnlockStatus.h"
#include "Globals.h"

#include "gui/ContentComponent.h"

#define EL_USE_AUDIO_PANEL 1

namespace Element {

class ContentComponent::Resizer : public StretchableLayoutResizerBar
{
public:
    Resizer (ContentComponent& contentComponent,
             StretchableLayoutManager* layoutToUse,
             int itemIndexInLayout,
             bool isBarVertical)
        : StretchableLayoutResizerBar (layoutToUse, itemIndexInLayout, isBarVertical),
          owner (contentComponent)
    {
        
    }
    
    void mouseDown (const MouseEvent& ev) override
    {
        StretchableLayoutResizerBar::mouseDown (ev);
        owner.resizerMouseDown();
    }
    
    void mouseUp (const MouseEvent& ev) override
    {
        StretchableLayoutResizerBar::mouseUp (ev);
        owner.resizerMouseUp();
    }
    
private:
    ContentComponent& owner;
};


class ContentComponent::Toolbar : public Component,
                                  public ButtonListener
{
public:
    Toolbar()
        : graph ("e"), title ("Session Name")
    {
        addAndMakeVisible (title);
        title.setText ("", dontSendNotification);
        
        addAndMakeVisible (graph);
        graph.setColour (TextButton::buttonColourId, Colors::toggleBlue.darker());
        graph.setColour (TextButton::buttonOnColourId, Colors::toggleBlue);
        graph.addListener (this);
        
        // addAndMakeVisible (trim);
        trim.setColour (Slider::rotarySliderFillColourId, LookAndFeel::elementBlue);
        trim.setName ("Trim");
        trim.setSliderStyle (Slider::RotaryVerticalDrag);
        trim.setRange (-70, 9.0);
    }
    
    ~Toolbar() { }
    
    void setSession (SessionPtr s)
    {
        session = s;
        if (session)
        {
            title.getTextValue().referTo (session->getNameValue());
        }
        resized();
    }
    
    void resized() override
    {
        Rectangle<int> r (getLocalBounds());
        r.removeFromLeft (10);
        graph.setBounds (r.removeFromLeft (graph.getHeight()).reduced (1 ,6));
        
        r.removeFromRight (10);
        trim.setBounds (r.removeFromRight (r.getHeight()).reduced(1));
        
        r.removeFromRight (10);
        title.setBounds (r);
    }
    
    void paint (Graphics& g) override
    {
        g.setColour (LookAndFeel_KV1::contentBackgroundColor.brighter(0.1));
        g.fillRect (getLocalBounds());
    }
    
    void buttonClicked (Button* btn) override
    {
        if (btn == &graph) {
            ViewHelpers::invokeDirectly (this, Commands::showPatchBay, true);
        }
    }

private:
    SessionPtr session;
    TextButton graph;
    Label title;
    Slider trim;
    Label dbLabel;
};

class ContentComponent::StatusBar : public Component,
                                    public Value::Listener,
                                    private Timer
{
public:
    StatusBar (Globals& g)
        : devices (g.getDeviceManager()),
          plugins (g.getPluginManager())
    {
        sampleRate.addListener (this);
        streamingStatus.addListener (this);
        status.addListener (this);
        
        addAndMakeVisible (sampleRateLabel);
        addAndMakeVisible (streamingStatusLabel);
        addAndMakeVisible (statusLabel);
        
        const Colour labelColor (0xffaaaaaa);
        const Font font (12.0f);
        
        for (int i = 0; i < getNumChildComponents(); ++i)
        {
            if (Label* label = dynamic_cast<Label*> (getChildComponent (i)))
            {
                label->setFont (font);
                label->setColour (Label::textColourId, labelColor);
                label->setJustificationType (Justification::centredLeft);
            }
        }
        
        startTimer (5000);
        updateLabels();
    }
    
    ~StatusBar()
    {
        sampleRate.removeListener (this);
        streamingStatus.removeListener (this);
        status.removeListener (this);
    }
    
    void paint (Graphics& g) override
    {
        g.setColour (LookAndFeel_KV1::contentBackgroundColor.brighter(0.1));
        g.fillRect (getLocalBounds());
        
        const Colour lineColor (0xff545454);
        g.setColour (lineColor);
        
        g.drawLine(streamingStatusLabel.getX(), 0, streamingStatusLabel.getX(), getHeight());
        g.drawLine(sampleRateLabel.getX(), 0, sampleRateLabel.getX(), getHeight());
        g.setColour (lineColor.darker());
        g.drawLine (0, 0, getWidth(), 0);
        g.setColour (lineColor);
        g.drawLine (0, 1, getWidth(), 1);
    }
    
    void resized() override
    {
        Rectangle<int> r (getLocalBounds());
        statusLabel.setBounds (r.removeFromLeft (getWidth() / 5));
        streamingStatusLabel.setBounds (r.removeFromLeft (r.getWidth() / 2));
        sampleRateLabel.setBounds(r);
    }
    
    void valueChanged (Value&) override
    {
        updateLabels();
    }
    
    void updateLabels()
    {
        if (auto* dev = devices.getCurrentAudioDevice())
        {
            devices.getCpuUsage();
            
            String text = "Sample Rate: ";
            text << String (dev->getCurrentSampleRate() * 0.001, 1) << " KHz";
            text << ":  Buffer: " << dev->getCurrentBufferSizeSamples();
            sampleRateLabel.setText (text, dontSendNotification);
            
            text.clear();
            String strText = streamingStatus.getValue().toString();
            if (strText.isEmpty())
                strText = "Running";
            text << "Engine: " << strText << ":  CPU: " << String(devices.getCpuUsage() * 100.f, 1) << "%";
            streamingStatusLabel.setText (text, dontSendNotification);
            
            statusLabel.setText (String("Device: ") + dev->getName(), dontSendNotification);
        }
        else
        {
            sampleRateLabel.setText ("", dontSendNotification);
            streamingStatusLabel.setText ("", dontSendNotification);
            statusLabel.setText ("No Device", dontSendNotification);
        }
    }
    
private:
    DeviceManager& devices;
    PluginManager& plugins;
    
    Label sampleRateLabel, streamingStatusLabel, statusLabel;
    ValueTree node;
    Value sampleRate, streamingStatus, status;
    
    friend class Timer;
    void timerCallback() override {
        updateLabels();
    }
};

class NavigationConcertinaPanel : public ConcertinaPanel {
public:
    NavigationConcertinaPanel (Globals& g)
        : globals(g),
          headerHeight (30),
          defaultPanelHeight (80)
    {
        setLookAndFeel (&lookAndFeel);
        updateContent();
    }
    
    ~NavigationConcertinaPanel()
    {
        setLookAndFeel (nullptr);
    }
    
    int getIndexOfPanel (Component* panel)
    {
        if (nullptr == panel)
            return -1;
        for (int i = 0; i < getNumPanels(); ++i)
            if (auto* p = getPanel (i))
                if (p == panel)
                    return i;
        return -1;
    }
    
    template<class T> T* findPanel() {
        for (int i = getNumPanels(); --i >= 0;)
            if (auto* panel = dynamic_cast<T*> (getPanel (i)))
                return panel;
        return nullptr;
    }
    
    void clearPanels()
    {
        for (int i = 0; i < comps.size(); ++i)
            removePanel (comps.getUnchecked (i));
        comps.clearQuick (true);
    }
    
    void updateContent()
    {
        clearPanels();
        Component* c = nullptr;
        c = new SessionGraphsListBox();
        auto *h = new ElementsHeader (*this, *c);
        addPanelInternal (-1, c, "Elements", h);
//        names.add ("Elements");
//        c = new SessionGraphsListBox();
//        addPanel (-1, c, true);
//        setPanelHeaderSize (c, headerHeight);
//        setCustomPanelHeader (c, new Header (*this), true);
    }
    
    AudioIOPanelView* getAudioIOPanel() { return findPanel<AudioIOPanelView>(); }
    PluginsPanelView* getPluginsPanel() { return findPanel<PluginsPanelView>(); }
    SessionGraphsListBox* getSessionPanel() { return findPanel<SessionGraphsListBox>(); }
    
    const StringArray& getNames() const { return names; }
    const int getHeaderHeight() const { return headerHeight; }
    void setHeaderHeight (const int newHeight)
    {
        jassert (newHeight > 0);
        headerHeight = newHeight;
        updateContent();
    }
    
private:
    typedef Element::LookAndFeel ELF;
    Globals& globals;
    int headerHeight;
    int defaultPanelHeight;
    
    StringArray names;
    OwnedArray<Component> comps;
    void addPanelInternal (const int index, Component* comp, const String& name = String(),
                           Component* header = nullptr)
    {
        jassert(comp);
        if (name.isNotEmpty())
            comp->setName (name);
        addPanel (index, comps.insert(index, comp), false);
        setPanelHeaderSize (comp, headerHeight);
        if (!header)
            header = new Header (*this, *comp);
        setCustomPanelHeader (comp, header, true);
    }
    
    class Header : public Component
    {
    public:
        Header (NavigationConcertinaPanel& _parent, Component& _panel)
            : parent(_parent), panel(_panel)
        {
            addAndMakeVisible (text);
            text.setColour (Label::textColourId, ELF::textColor);
        }
        
        virtual ~Header() { }

        virtual void resized() override
        {
            text.setBounds (4, 1, 100, getHeight() - 2);
        }
        
        virtual void paint (Graphics& g) override
        {
            getLookAndFeel().drawConcertinaPanelHeader (
                g, getLocalBounds(), false, false, parent, panel);
        }
        
    protected:
        NavigationConcertinaPanel& parent;
        Component& panel;
        Label text;
    };
    
    class ElementsHeader : public Header,
                            public ButtonListener
    {
    public:
        ElementsHeader (NavigationConcertinaPanel& _parent, Component& _panel)
            : Header (_parent, _panel)
        {
            addAndMakeVisible (addButton);
            addButton.setButtonText ("+");
            addButton.addListener (this);
            setInterceptsMouseClicks (false, true);
            
        }
        
        void resized() override
        {
            const int padding = 4;
            const int buttonSize = getHeight() - (padding * 2);
            addButton.setBounds (getWidth() - padding - buttonSize,
                                 padding, buttonSize, buttonSize);
        }
        
        void buttonClicked (Button*) override
        {
            if (auto* cc = findParentComponentOfClass<ContentComponent>())
                cc->getGlobals().getCommandManager().invokeDirectly (
                    (int)Commands::sessionAddGraph, true);
        }
        
    private:
        TextButton addButton;
    };
    
    class LookAndFeel : public Element::LookAndFeel
    {
    public:
        LookAndFeel() { }
        ~LookAndFeel() { }
        
        void drawConcertinaPanelHeader (Graphics& g, const Rectangle<int>& area,
                                        bool isMouseOver, bool isMouseDown,
                                        ConcertinaPanel& panel, Component& comp)
        {
#if 0
            auto* p = dynamic_cast<NavigationConcertinaPanel*> (&panel);
            int i = p->getNumPanels();
            while (--i >= 0) {
                if (p->getPanel(i) == &comp)
                    break;
            }
#endif
            ELF::drawConcertinaPanelHeader (g, area, isMouseOver, isMouseDown, panel, comp);
            g.setColour (Colours::white);
            Rectangle<int> r (area.withTrimmedLeft (20));
            g.drawText (comp.getName(), 20, 0, r.getWidth(), r.getHeight(),
                        Justification::centredLeft);
        }
    } lookAndFeel;
};
    
class ContentContainer : public Component
{
public:
    ContentContainer (ContentComponent& cc, AppController& app)
        : owner (cc)
    {
        addAndMakeVisible (content1 = new ContentView());
        addAndMakeVisible (bar = new StretchableLayoutResizerBar (&layout, 1, false));
        addAndMakeVisible (content2 = new ContentView());
        updateLayout();
        resized();
    }
    
    virtual ~ContentContainer() { }
    
    void resized() override
    {
        Component* comps[] = { 0, bar.get(), 0 };
        comps[0] = content1.get();
        comps[2] = content2.get();
        layout.layOutComponents (comps, 3, 0, 0, getWidth(), getHeight(), true, true);
    }
    
    void setNode (const Node& node)
    {
        if (auto* grid = dynamic_cast<ConnectionGrid*> (content1.get()))
            grid->setNode (node);
    }
    
    void setMainView (ContentView* view)
    {
        if (content1)
            removeChildComponent (content1);
        content1 = view;
        if (content1)
        {
            addAndMakeVisible (content1);
            content1->willBecomeActive();
        }
        resized();
    }
    
    void setAccessoryView (ContentView* view)
    {
        if (content2)
            removeChildComponent (content2);
        content1 = view;
        if (content2)
            addAndMakeVisible (content2);
        resized();
    }
    
private:
    ContentComponent& owner;
    StretchableLayoutManager layout;
    ScopedPointer<StretchableLayoutResizerBar> bar;
    ScopedPointer<ContentView> content1;
    ScopedPointer<ContentView> content2;
    
    void updateLayout()
    {
        layout.setItemLayout (0, 200, -1.0, 200);
        layout.setItemLayout (1, 0, 0, 0);
        layout.setItemLayout (2, 0, -1.0, 0);
    }
};

ContentComponent::ContentComponent (AppController& ctl_)
    : controller (ctl_)
{
    setOpaque (true);
    
    addAndMakeVisible (nav = new NavigationConcertinaPanel (ctl_.getWorld()));
    addAndMakeVisible (bar1 = new Resizer (*this, &layout, 1, true));
    addAndMakeVisible (container = new ContentContainer (*this, controller));
    addAndMakeVisible (statusBar = new StatusBar (getGlobals()));
    addAndMakeVisible (toolBar = new Toolbar());
    
    container->setMainView (new ConnectionGrid ());
    const Node node (getGlobals().getSession()->getGraph(0));
    setCurrentNode (node);
    
    toolBarVisible = true;
    toolBarSize = 32;
    statusBarVisible = true;
    statusBarSize = 22;
    
    setSize (600, 600);
    updateLayout();
    resized();
    
    nav->expandPanelFully (nav->getSessionPanel(), false);
}

ContentComponent::~ContentComponent()
{
    toolTips = nullptr;
}

Globals& ContentComponent::getGlobals() { return controller.getGlobals(); }
SessionPtr ContentComponent::getSession() { return getGlobals().getSession(); }
    
void ContentComponent::childBoundsChanged (Component* child)
{
}

void ContentComponent::mouseDown (const MouseEvent& ev)
{
    Component::mouseDown (ev);
}

void ContentComponent::paint (Graphics &g)
{
    g.fillAll (LookAndFeel::backgroundColor);
}

void ContentComponent::resized()
{
    Rectangle<int> r (getLocalBounds());
    
    if (toolBarVisible)
        toolBar->setBounds (r.removeFromTop (toolBarSize));
    if (statusBarVisible)
        statusBar->setBounds (r.removeFromBottom (statusBarSize));
    
    Component* comps[3] = { nav.get(), bar1.get(), container.get() };
    layout.layOutComponents (comps, 3, r.getX(), r.getY(),
                             r.getWidth(), r.getHeight(),
                             false, true);
}

bool ContentComponent::isInterestedInFileDrag (const StringArray &files)
{
    for (const auto& path : files)
    {
        const File file (path);
        if (file.hasFileExtension ("elc;elg;els"))
            return true;
    }
    return false;
}

void ContentComponent::filesDropped (const StringArray &files, int x, int y)
{
    for (const auto& path : files)
    {
        const File file (path);
        if (file.hasFileExtension ("elc"))
        {
            auto& unlock (controller.getGlobals().getUnlockStatus());
            FileInputStream src (file);
            if (unlock.applyKeyFile (src.readString()))
            {
                AlertWindow::showMessageBox (AlertWindow::InfoIcon,
                    "Apply License File", "Your software has successfully been unlocked.");
            }
            else
            {
                AlertWindow::showMessageBox (AlertWindow::InfoIcon,
                    "Apply License File", "Your software could not be unlocked.");
            }
        }
    }
}

void ContentComponent::post (Message* message)
{
    controller.postMessage (message);
}

void ContentComponent::stabilize()
{
    auto session = getGlobals().getSession();
    setCurrentNode (session->getGraph (0));
    if (auto* window = findParentComponentOfClass<DocumentWindow>())
        window->setName ("Element - " + session->getName());
    if (auto* sp = nav->getSessionPanel())
        sp->setSession (session);
    toolBar->setSession (session);
}

void ContentComponent::setCurrentNode (const Node& node)
{
    if (auto* audioPanel = nav->getAudioIOPanel())
        audioPanel->setNode (node);
    if (node.hasNodeType (Tags::graph))
        container->setNode (node);
}

void ContentComponent::updateLayout()
{
    layout.setItemLayout (0, 220, 220, 220);
    layout.setItemLayout (1, 4, 4, 4);
    layout.setItemLayout (2, 300, -1, 400);
}

void ContentComponent::resizerMouseDown()
{
    layout.setItemLayout (0, 220, 220, 220);
    layout.setItemLayout (1, 4, 4, 4);
    layout.setItemLayout (2, 300, -1, 400);
    resized();
}

void ContentComponent::resizerMouseUp()
{
    layout.setItemLayout (0, 220, 220, 220);
    layout.setItemLayout (1, 4, 4, 4);
    layout.setItemLayout (2, 300, -1, 400);
    resized();
}
    
void ContentComponent::setContentView (ContentView* view)
{
    jassert (view && container);
    ScopedPointer<ContentView> deleter (view);
    container->setMainView (deleter.release());
}
}


