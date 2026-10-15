#pragma once
#include <JuceHeader.h>

// Classic hardware-synth look: charcoal brushed panels, boxed sections with a
// title strip, chrome-style knobs with a lit value arc, and a backlit LCD.
namespace Look
{
    const juce::Colour bgTop    { 0xff34373c };
    const juce::Colour bgBottom { 0xff1d1f22 };
    const juce::Colour panel    { 0xff2a2d31 };
    const juce::Colour panelEdge{ 0xff4a4f56 };
    const juce::Colour strip    { 0xff17191c };
    const juce::Colour title    { 0xffb6c0cc };
    const juce::Colour label    { 0xff9aa5b1 };
    const juce::Colour accent   { 0xff4aa3ff };
    const juce::Colour lcdBg    { 0xff0a1118 };
    const juce::Colour lcdText  { 0xff8fd3ff };
    const juce::Colour lcdDim   { 0xff1e3447 };

    inline juce::Font font(float size, bool bold = false)
    {
        return juce::Font(juce::FontOptions(size, bold ? juce::Font::bold : juce::Font::plain));
    }

    // Boxed section with a dark title strip.
    inline void drawPanel(juce::Graphics& g, juce::Rectangle<float> r, const juce::String& text)
    {
        g.setGradientFill(juce::ColourGradient(panel.brighter(0.08f), r.getX(), r.getY(),
                                               panel.darker(0.25f), r.getX(), r.getBottom(), false));
        g.fillRoundedRectangle(r, 5.0f);

        auto head = r.withHeight(19.0f);
        g.setColour(strip);
        g.fillRoundedRectangle(head.getX(), head.getY(), head.getWidth(), head.getHeight(), 5.0f);
        g.fillRect(head.withTop(head.getY() + 8.0f));

        g.setColour(title);
        g.setFont(font(10.5f, true));
        g.drawText(text, head.reduced(8.0f, 0.0f), juce::Justification::centredLeft, false);

        g.setColour(juce::Colours::black.withAlpha(0.6f));
        g.drawRoundedRectangle(r, 5.0f, 1.2f);
        g.setColour(panelEdge.withAlpha(0.55f));
        g.drawRoundedRectangle(r.reduced(1.0f), 4.0f, 1.0f);
    }

    inline void drawLcd(juce::Graphics& g, juce::Rectangle<float> r)
    {
        g.setGradientFill(juce::ColourGradient(lcdBg.brighter(0.05f), r.getX(), r.getY(),
                                               lcdBg, r.getX(), r.getBottom(), false));
        g.fillRoundedRectangle(r, 4.0f);
        g.setColour(juce::Colours::black);
        g.drawRoundedRectangle(r, 4.0f, 1.5f);
        g.setColour(lcdDim.withAlpha(0.8f));
        g.drawRoundedRectangle(r.reduced(1.5f), 3.0f, 1.0f);
    }
}

class SynthLookAndFeel : public juce::LookAndFeel_V4
{
public:
    SynthLookAndFeel()
    {
        setColour(juce::Slider::rotarySliderFillColourId, Look::accent);
        setColour(juce::ComboBox::backgroundColourId, juce::Colours::transparentBlack);
        setColour(juce::ComboBox::outlineColourId, juce::Colours::transparentBlack);
        setColour(juce::ComboBox::textColourId, Look::lcdText);
        setColour(juce::ComboBox::arrowColourId, Look::lcdText);
        setColour(juce::PopupMenu::backgroundColourId, Look::lcdBg);
        setColour(juce::PopupMenu::textColourId, Look::lcdText);
        setColour(juce::PopupMenu::highlightedBackgroundColourId, Look::accent.darker(0.4f));
        setColour(juce::PopupMenu::highlightedTextColourId, juce::Colours::white);
        setColour(juce::TextButton::textColourOffId, Look::title);
        setColour(juce::TextButton::textColourOnId, juce::Colours::white);
    }

    void drawRotarySlider(juce::Graphics& g, int x, int y, int w, int h, float pos,
                          float startAngle, float endAngle, juce::Slider& s) override
    {
        auto bounds = juce::Rectangle<float>((float) x, (float) y, (float) w, (float) h).reduced(2.0f);
        const float d = juce::jmin(bounds.getWidth(), bounds.getHeight());
        const auto c = bounds.getCentre();
        const float r = d * 0.5f;
        const float angle = startAngle + pos * (endAngle - startAngle);
        const float ringR = r - 1.5f;

        // Value track and lit arc.
        juce::Path track;
        track.addCentredArc(c.x, c.y, ringR, ringR, 0.0f, startAngle, endAngle, true);
        g.setColour(juce::Colours::black.withAlpha(0.7f));
        g.strokePath(track, juce::PathStrokeType(3.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        juce::Path value;
        value.addCentredArc(c.x, c.y, ringR, ringR, 0.0f, startAngle, juce::jmax(startAngle + 0.001f, angle), true);
        const auto accent = s.findColour(juce::Slider::rotarySliderFillColourId);
        g.setColour(accent.withAlpha(0.35f));
        g.strokePath(value, juce::PathStrokeType(5.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour(accent);
        g.strokePath(value, juce::PathStrokeType(2.4f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // Chrome-ish knob body.
        const float br = r - 6.0f;
        g.setColour(juce::Colours::black.withAlpha(0.55f));
        g.fillEllipse(c.x - br, c.y - br + 2.0f, br * 2.0f, br * 2.0f);
        g.setGradientFill(juce::ColourGradient(juce::Colour(0xff7a808a), c.x - br * 0.5f, c.y - br * 0.8f,
                                               juce::Colour(0xff1b1d21), c.x + br * 0.6f, c.y + br * 0.9f, true));
        g.fillEllipse(c.x - br, c.y - br, br * 2.0f, br * 2.0f);
        g.setColour(juce::Colour(0xff0b0c0e));
        g.drawEllipse(c.x - br, c.y - br, br * 2.0f, br * 2.0f, 1.3f);
        const float cap = br * 0.62f;
        g.setGradientFill(juce::ColourGradient(juce::Colour(0xff3c4046), c.x, c.y - cap,
                                               juce::Colour(0xff25272b), c.x, c.y + cap, false));
        g.fillEllipse(c.x - cap, c.y - cap, cap * 2.0f, cap * 2.0f);

        // Pointer.
        juce::Path p;
        p.addRoundedRectangle(-1.3f, -br + 1.5f, 2.6f, br * 0.55f, 1.0f);
        g.setColour(juce::Colour(0xfff2f6fa));
        g.fillPath(p, juce::AffineTransform::rotation(angle).translated(c.x, c.y));
    }

    void drawButtonBackground(juce::Graphics& g, juce::Button& b, const juce::Colour&,
                              bool highlighted, bool down) override
    {
        auto r = b.getLocalBounds().toFloat().reduced(0.5f);
        auto top = juce::Colour(0xff4b5058), bottom = juce::Colour(0xff2a2d32);
        if (highlighted) { top = top.brighter(0.15f); bottom = bottom.brighter(0.1f); }
        if (down) std::swap(top, bottom);
        g.setGradientFill(juce::ColourGradient(top, 0, r.getY(), bottom, 0, r.getBottom(), false));
        g.fillRoundedRectangle(r, 4.0f);
        g.setColour(juce::Colours::black.withAlpha(0.8f));
        g.drawRoundedRectangle(r, 4.0f, 1.2f);
    }

    juce::Font getTextButtonFont(juce::TextButton&, int) override { return Look::font(13.0f, true); }
};
