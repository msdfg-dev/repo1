#include "PluginEditor.h"

// ---------------------------------------------------------------------------
// OscWindow
// ---------------------------------------------------------------------------
OscWindow::OscWindow(juce::AudioProcessorValueTreeState& apvts, Type type, const juce::String& prefix,
                     const juce::String& title, const juce::String& shapeName)
    : oscType(type), titleText(title)
{
    shapeParam = apvts.getRawParameterValue(prefix + "_shape");
    levelParam = apvts.getRawParameterValue(prefix + "_level");

    setupKnob(knobs[0], apvts, prefix + "_level", "LEVEL");
    setupKnob(knobs[1], apvts, prefix + "_oct", "OCTAVE");
    setupKnob(knobs[2], apvts, prefix + "_fine", "FINE");
    setupKnob(knobs[3], apvts, prefix + "_shape", shapeName);

    startTimerHz(20);
}

void OscWindow::setupKnob(Knob& k, juce::AudioProcessorValueTreeState& apvts, const juce::String& paramId,
                          const juce::String& text)
{
    k.slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    k.slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    k.slider.setPopupDisplayEnabled(true, true, nullptr);
    addAndMakeVisible(k.slider);

    k.label.setText(text, juce::dontSendNotification);
    k.label.setJustificationType(juce::Justification::centred);
    k.label.setFont(Look::font(9.5f, true));
    k.label.setColour(juce::Label::textColourId, Look::label);
    k.label.setInterceptsMouseClicks(false, false);
    addAndMakeVisible(k.label);

    k.attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, paramId, k.slider);
}

void OscWindow::timerCallback()
{
    const float s = shapeParam->load(), l = levelParam->load();
    if (s != lastShape || l != lastLevel)
    {
        lastShape = s;
        lastLevel = l;
        repaint(displayArea);
    }
}

void OscWindow::resized()
{
    auto r = getLocalBounds();
    displayArea = juce::Rectangle<int>(8, 26, r.getWidth() - 16, 52);

    const int cw = (r.getWidth() - 16) / 4;
    for (int i = 0; i < 4; ++i)
    {
        const int x = 8 + i * cw;
        knobs[(size_t) i].slider.setBounds(x, 82, cw, 48);
        knobs[(size_t) i].label.setBounds(x, 131, cw, 14);
    }
}

void OscWindow::paint(juce::Graphics& g)
{
    Look::drawPanel(g, getLocalBounds().toFloat(), titleText);

    // Backlit waveform display (two cycles).
    auto d = displayArea.toFloat();
    Look::drawLcd(g, d);

    auto inner = d.reduced(4.0f, 5.0f);
    g.setColour(Look::lcdDim);
    g.drawHorizontalLine((int) inner.getCentreY(), inner.getX(), inner.getRight());
    for (int i = 1; i < 4; ++i)
    {
        const float x = inner.getX() + inner.getWidth() * (float) i / 4.0f;
        g.drawVerticalLine((int) x, inner.getY(), inner.getBottom());
    }

    const float shape = shapeParam->load();
    const float level = levelParam->load();

    juce::Path wave;
    const int n = juce::jmax(2, (int) inner.getWidth());
    for (int i = 0; i < n; ++i)
    {
        const double phase = std::fmod(2.0 * (double) i / (double) (n - 1), 1.0);
        float v = 0.0f;
        switch (oscType)
        {
            case Type::Sine:     v = Wave::sine(phase, shape); break;
            case Type::Triangle: v = Wave::triangle(phase, shape); break;
            case Type::Saw:      v = Wave::saw(phase, shape, 0.0); break;
        }
        const float px = inner.getX() + (float) i;
        const float py = inner.getCentreY() - v * inner.getHeight() * 0.46f;
        if (i == 0) wave.startNewSubPath(px, py); else wave.lineTo(px, py);
    }

    const float alpha = 0.3f + 0.7f * juce::jlimit(0.0f, 1.0f, level);
    g.setColour(Look::lcdText.withAlpha(alpha * 0.3f));
    g.strokePath(wave, juce::PathStrokeType(3.5f));
    g.setColour(Look::lcdText.withAlpha(alpha));
    g.strokePath(wave, juce::PathStrokeType(1.4f));
}

// ---------------------------------------------------------------------------
// Editor
// ---------------------------------------------------------------------------
NocturneDarkSynthAudioProcessorEditor::NocturneDarkSynthAudioProcessorEditor(NocturneDarkSynthAudioProcessor& p)
    : AudioProcessorEditor(&p), processor(p),
      sineWindow(p.parameters, OscWindow::Type::Sine, "sine", "OSC 1  -  SINE", "FOLD"),
      triWindow(p.parameters, OscWindow::Type::Triangle, "tri", "OSC 2  -  TRIANGLE", "SKEW"),
      sawWindow(p.parameters, OscWindow::Type::Saw, "saw", "OSC 3  -  SAW", "SQUARE"),
      keyboard(p.keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard)
{
    setLookAndFeel(&laf);

    addAndMakeVisible(sineWindow);
    addAndMakeVisible(triWindow);
    addAndMakeVisible(sawWindow);

    addKnob("CUTOFF", "cutoff");  addKnob("RESONANCE", "resonance"); addKnob("DRIVE", "drive");
    addKnob("ATTACK", "attack");  addKnob("RELEASE", "release");
    addKnob("MIX", "phaser");     addKnob("RATE", "rate");           addKnob("DEPTH", "depth");

    // Sound banks in the LCD.
    for (auto& bank : Presets::banks())
        bankBox.addItem(bank.name, bankBox.getNumItems() + 1);
    addAndMakeVisible(bankBox);
    addAndMakeVisible(patchBox);
    addAndMakeVisible(prevButton);
    addAndMakeVisible(nextButton);

    bankBox.onChange = [this] {
        const int bank = bankBox.getSelectedItemIndex();
        if (bank < 0) return;
        fillPatchBox(bank);
        processor.applyPatch(Presets::indexOf(bank, 0));
        syncPatchUi();
    };
    patchBox.onChange = [this] {
        const int bank = bankBox.getSelectedItemIndex(), patch = patchBox.getSelectedItemIndex();
        if (bank >= 0 && patch >= 0)
            processor.applyPatch(Presets::indexOf(bank, patch));
    };
    prevButton.onClick = [this] { stepPatch(-1); };
    nextButton.onClick = [this] { stepPatch(+1); };

    keyboard.setKeyWidth(26.0f);
    keyboard.setAvailableRange(24, 108);
    keyboard.setLowestVisibleKey(36);
    keyboard.setWantsKeyboardFocus(true);
    keyboard.setColour(juce::MidiKeyboardComponent::keyDownOverlayColourId, Look::accent.withAlpha(0.75f));
    keyboard.setColour(juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId, Look::accent.withAlpha(0.25f));
    keyboard.setColour(juce::MidiKeyboardComponent::shadowColourId, juce::Colours::black.withAlpha(0.6f));
    addAndMakeVisible(keyboard);

    syncPatchUi();
    startTimerHz(8);   // follow program changes coming from the host

    // Set the size last so resized() lays out the children that now exist.
    setSize(780, 460);
}

NocturneDarkSynthAudioProcessorEditor::~NocturneDarkSynthAudioProcessorEditor()
{
    setLookAndFeel(nullptr);
}

void NocturneDarkSynthAudioProcessorEditor::addKnob(const juce::String& title, const juce::String& id)
{
    auto* s = knobs.add(new juce::Slider());
    s->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    s->setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    s->setPopupDisplayEnabled(true, true, nullptr);
    addAndMakeVisible(s);

    auto* l = labels.add(new juce::Label());
    l->setText(title, juce::dontSendNotification);
    l->setJustificationType(juce::Justification::centred);
    l->setFont(Look::font(9.5f, true));
    l->setColour(juce::Label::textColourId, Look::label);
    l->setInterceptsMouseClicks(false, false);
    addAndMakeVisible(l);

    attachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.parameters, id, *s));
}

void NocturneDarkSynthAudioProcessorEditor::fillPatchBox(int bank)
{
    patchBox.clear(juce::dontSendNotification);
    int id = 1;
    for (auto& patch : Presets::banks()[(size_t) bank].patches)
        patchBox.addItem(patch.name, id++);
}

void NocturneDarkSynthAudioProcessorEditor::syncPatchUi()
{
    int bank = 0, patch = 0;
    Presets::locate(processor.getCurrentProgram(), bank, patch);

    if (bankBox.getSelectedItemIndex() != bank || patchBox.getNumItems() == 0)
    {
        bankBox.setSelectedItemIndex(bank, juce::dontSendNotification);
        fillPatchBox(bank);
    }
    if (patchBox.getSelectedItemIndex() != patch)
        patchBox.setSelectedItemIndex(patch, juce::dontSendNotification);
}

void NocturneDarkSynthAudioProcessorEditor::stepPatch(int delta)
{
    const int total = Presets::totalPatches();
    const int next = (processor.getCurrentProgram() + delta + total) % total;
    processor.applyPatch(next);
    syncPatchUi();
}

void NocturneDarkSynthAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.setGradientFill(juce::ColourGradient(Look::bgTop, 0, 0, Look::bgBottom, 0, (float) getHeight(), false));
    g.fillAll();

    // Fine horizontal grain for a brushed-metal feel.
    g.setColour(juce::Colours::white.withAlpha(0.018f));
    for (int y = 0; y < getHeight(); y += 3)
        g.drawHorizontalLine(y, 0.0f, (float) getWidth());

    // Header bar.
    auto hb = headerBar.toFloat();
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff3d4147), 0, hb.getY(),
                                           juce::Colour(0xff222427), 0, hb.getBottom(), false));
    g.fillRect(hb);
    g.setColour(juce::Colours::black.withAlpha(0.7f));
    g.drawHorizontalLine(headerBar.getBottom() - 1, 0.0f, (float) getWidth());
    g.setColour(Look::panelEdge.withAlpha(0.6f));
    g.drawHorizontalLine(0, 0.0f, (float) getWidth());

    // Logo.
    g.setColour(juce::Colours::black.withAlpha(0.7f));
    g.setFont(Look::font(25.0f, true));
    g.drawText("NOCTURNE", 17, 9, 220, 30, juce::Justification::left);
    g.setColour(juce::Colour(0xffe9eef4));
    g.drawText("NOCTURNE", 16, 8, 220, 30, juce::Justification::left);
    g.setColour(Look::accent);
    g.setFont(Look::font(9.5f, true));
    g.drawText("DARK  SYNTHESIZER", 18, 36, 200, 12, juce::Justification::left);

    // Preset LCD.
    Look::drawLcd(g, lcdArea.toFloat());
    g.setColour(Look::lcdDim);
    g.drawVerticalLine(bankBox.getRight() + 2, (float) lcdArea.getY() + 5.0f, (float) lcdArea.getBottom() - 5.0f);

    // Section panels.
    Look::drawPanel(g, filterPanel.toFloat(), "FILTER");
    Look::drawPanel(g, ampPanel.toFloat(), "AMP ENVELOPE");
    Look::drawPanel(g, phaserPanel.toFloat(), "PHASER");

    // Keyboard rail.
    g.setColour(juce::Colour(0xff0f1012));
    g.fillRect(keyboardBar);
    g.setColour(Look::panelEdge.withAlpha(0.5f));
    g.drawHorizontalLine(keyboardBar.getY(), 0.0f, (float) getWidth());
}

void NocturneDarkSynthAudioProcessorEditor::resized()
{
    const int m = 12, gap = 8, W = getWidth();
    auto area = getLocalBounds();
    headerBar = area.removeFromTop(54);

    // Preset display: [<] [ bank | patch ] [>]
    lcdArea = juce::Rectangle<int>(W / 2 - 160, 10, 320, 34);
    prevButton.setBounds(lcdArea.getX() - 34, 14, 28, 26);
    nextButton.setBounds(lcdArea.getRight() + 6, 14, 28, 26);
    bankBox.setBounds(lcdArea.getX() + 6, lcdArea.getY() + 3, 104, 28);
    patchBox.setBounds(bankBox.getRight() + 6, lcdArea.getY() + 3, lcdArea.getRight() - bankBox.getRight() - 12, 28);

    // Oscillator row.
    const int oscY = 62, oscH = 150;
    const int oscW = (W - 2 * m - 2 * gap) / 3;
    sineWindow.setBounds(m, oscY, oscW, oscH);
    triWindow.setBounds(m + oscW + gap, oscY, oscW, oscH);
    sawWindow.setBounds(m + 2 * (oscW + gap), oscY, W - m - (m + 2 * (oscW + gap)), oscH);

    // Filter / envelope / phaser row: 3 + 2 + 3 knob cells.
    const int rowY = oscY + oscH + gap, rowH = 100;
    const int cell = (W - 2 * m - 2 * gap) / 8;
    filterPanel = { m, rowY, cell * 3, rowH };
    ampPanel = { filterPanel.getRight() + gap, rowY, cell * 2, rowH };
    phaserPanel = { ampPanel.getRight() + gap, rowY, W - m - (ampPanel.getRight() + gap), rowH };

    auto place = [this, rowY](juce::Rectangle<int> panel, int firstKnob, int count) {
        const int cw = panel.getWidth() / count;
        for (int i = 0; i < count; ++i)
        {
            const int x = panel.getX() + i * cw;
            knobs[firstKnob + i]->setBounds(x, rowY + 24, cw, 50);
            labels[firstKnob + i]->setBounds(x, rowY + 75, cw, 14);
        }
    };
    place(filterPanel, 0, 3);
    place(ampPanel, 3, 2);
    place(phaserPanel, 5, 3);

    // Keyboard.
    const int kbY = rowY + rowH + gap;
    keyboardBar = juce::Rectangle<int>(0, kbY, W, getHeight() - kbY);
    keyboard.setBounds(keyboardBar.reduced(m, 8));
}
