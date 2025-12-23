// P2STMap.cpp
// Converts a position pass (P) to STMap using camera matrices
// Applies: inverse_transform -> projection (w_divide) -> format (w_divide) -> normalize

static const char* const HELP = 
    "Converts a position pass (P) to STMap coordinates using camera matrices.\n"
    "Applies camera inverse transform, projection, and format matrices in sequence.\n"
    "More efficient than chaining three C44Matrix nodes.\n";

#include "DDImage/PixelIop.h"
#include "DDImage/CameraOp.h"
#include "DDImage/Row.h"
#include "DDImage/Knobs.h"
#include "DDImage/Matrix4.h"

using namespace DD::Image;

class P2STMap : public PixelIop
{
    Matrix4 _cam_transform_inv;
    Matrix4 _cam_projection;
    Matrix4 _cam_format;
    float _format_width;
    float _format_height;

public:
    P2STMap(Node* node) : PixelIop(node),
        _format_width(1.0f),
        _format_height(1.0f)
    {
        _cam_transform_inv.makeIdentity();
        _cam_projection.makeIdentity();
        _cam_format.makeIdentity();
    }

    bool pass_transform() const { return true; }
    
    virtual int minimum_inputs() const { return 2; } // img + cam
    virtual int maximum_inputs() const { return 2; }
    
    virtual void knobs(Knob_Callback f);
    
    static const Iop::Description d;
    const char* Class() const { return d.name; }
    const char* node_help() const { return HELP; }

    void _validate(bool);
    void _request(int x, int y, int r, int t, ChannelMask channels, int count);
    
    void in_channels(int input, ChannelSet& mask) const {
        if (input == 0) {
            mask += Mask_RGBA;
        }
    }

    void pixel_engine(const Row& in, int y, int x, int r, ChannelMask channels, Row& out);

    bool test_input(int n, Op* op) const {
        if (n >= 1) {
            return dynamic_cast<CameraOp*>(op) != 0;
        }
        return Iop::test_input(n, op);
    }

    Op* default_input(int input) const {
        if (input == 1) {
            return CameraOp::default_camera();
        }
        return Iop::default_input(input);
    }

    const char* input_label(int input, char* buffer) const {
        switch (input) {
            case 0: return "P";
            case 1: return "cam";
        }
        return nullptr;
    }
};

void P2STMap::_validate(bool for_real)
{
    copy_info();

    // Get camera and validate it
    CameraOp* cam_op = dynamic_cast<CameraOp*>(Op::input(1));
    if (cam_op) {
        cam_op->validate(for_real);
        
        // Get only the matrices that don't depend on output context
        _cam_transform_inv = cam_op->matrix().inverse();
        _cam_projection = cam_op->projection();
        
        // Store format dimensions for normalization
        const Format& fmt = info_.format();
        _format_width = static_cast<float>(fmt.width());
        _format_height = static_cast<float>(fmt.height());
    } else {
        // No camera - use identity matrices
        _cam_transform_inv.makeIdentity();
        _cam_projection.makeIdentity();
        _format_width = 1.0f;
        _format_height = 1.0f;
    }

    // Output RGBA channels (r=u, g=v, b=0, a=1 typically)
    set_out_channels(Mask_RGBA);
    info_.turn_on(Mask_RGBA);
    info_.black_outside(true);
}

void P2STMap::_request(int x, int y, int r, int t, ChannelMask channels, int count)
{
    // Request RGBA from input (position pass)
    ChannelSet request_chans = Mask_RGBA;
    input0().request(x, y, r, t, request_chans, count);
}

void P2STMap::pixel_engine(const Row& in, int y, int x, int r, ChannelMask channels, Row& out)
{
    if (aborted())
        return;

    // Get format matrix (needs to be done per-render, not in validate)
    Matrix4 cam_format;
    cam_format.makeIdentity();
    CameraOp* cam_op = dynamic_cast<CameraOp*>(Op::input(1));
    if (cam_op) {
        cam_op->to_format(cam_format, &info_.format());
    }

    // Input channels (position pass P)
    const float* R = in[Chan_Red];
    const float* G = in[Chan_Green];
    const float* B = in[Chan_Blue];
    const float* A = in[Chan_Alpha];

    // Output channels (STMap)
    float* outR = out.writable(Chan_Red);
    float* outG = out.writable(Chan_Green);
    float* outB = out.writable(Chan_Blue);
    float* outA = out.writable(Chan_Alpha);

    // Process each pixel
    for (int X = x; X < r; X++) {
        // Read input position
        Vector4 P(R[X], G[X], B[X], A[X]);
        
        // Step 1: Apply inverse transform (NO w_divide)
        Vector4 v1 = _cam_transform_inv.transform(P);
        
        // Step 2: Apply projection WITH w_divide
        Vector4 v2 = _cam_projection.transform(v1);
        if (v2.w != 0.0f) {
            v2 /= v2.w;
        }
        
        // Step 3: Apply format matrix WITH w_divide
        Vector4 v3 = cam_format.transform(v2);
        if (v3.w != 0.0f) {
            v3 /= v3.w;
        }
        
        // Step 4: Normalize by format dimensions (Expression: r/width, g/height)
        outR[X] = v3.x / _format_width;
        outG[X] = v3.y / _format_height;
        outB[X] = v3.z;
        outA[X] = v3.w;
    }
}

void P2STMap::knobs(Knob_Callback f)
{
    // Could add options here later, like:
    // - toggle w_divide on/off for each stage
    // - output format selection
    // - etc.
    
    Divider(f, "");
    Text_knob(f, "P2STMap by Peter Mercell 2025\nInspired by Ivan Busquets's C44Matrix");
}

static Iop* build(Node* node) { 
    return new P2STMap(node); 
}

const Iop::Description P2STMap::d("P2STMap", "Transform/P2STMap", build);
