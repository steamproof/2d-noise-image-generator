#include "args.h"
#include "colormap.h"
#include "util.h"
#include "FastNoiseLite.h"
#include "stb_image_write.h"
#include <cmath>
#include <cstdio>
#include <stdexcept>
#include <string>
#include <vector>

static void help() {
    std::printf("noise\n");
    std::printf("usage:\n");
    std::printf("  noise [options]\n\n");
    std::printf("core:\n");
    std::printf("  --width <int> (default 512)\n");
    std::printf("  --height <int> (default 512)\n");
    std::printf("  --seed <int> (default 0)\n");
    std::printf("  --freq <float> (alias --scale) (default 0.01)\n");
    std::printf("  --z <float> (default 0)\n");
    std::printf("  --type <OpenSimplex2|OpenSimplex2S|Perlin|Value|ValueCubic|Cellular> (default Perlin)\n");
    std::printf("  --type simplex (alias for OpenSimplex2S)\n");
    std::printf("  --rotation3d <None|ImproveXYPlanes|ImproveXZPlanes> (default None)\n");
    std::printf("tile:\n");
    std::printf("  --tile (default off)\n");
    std::printf("  --tile-period <int> (default width)\n");
    std::printf("fractal:\n");
    std::printf("  --fractal-type <None|FBm|Rigid|PingPong> (default None)\n");
    std::printf("  --octaves <int> (default 5)\n");
    std::printf("  --gain <float> (default 0.5)\n");
    std::printf("  --lacunarity <float> (default 2.0)\n");
    std::printf("  --weighted-strength <float> (default 0.0)\n");
    std::printf("  --pingpong-strength <float> (default 2.0)\n");
    std::printf("cellular (only when --type Cellular):\n");
    std::printf("  --cell-dist <Euclidean|EuclideanSq|Manhattan|Hybrid> (default Euclidean)\n");
    std::printf("  --cell-return <CellValue|Distance|Distance2|Distance2Add|Distance2Sub|Distance2Mul|Distance2Div> (default Distance)\n");
    std::printf("  --cell-jitter <float> (default 1.0)\n");
    std::printf("domain warp:\n");
    std::printf("  --warp (default off)\n");
    std::printf("  --warp-type <OpenSimplex2|OpenSimplex2Reduced|BasicGrid> (default OpenSimplex2)\n");
    std::printf("  --warp-amp <float> (default 1.0)\n");
    std::printf("  --warp-seed <int> (default seed+1)\n");
    std::printf("  --warp-freq <float> (default same as --freq)\n");
    std::printf("  --warp-rotation3d <None|ImproveXYPlanes|ImproveXZPlanes> (default same as --rotation3d)\n");
    std::printf("  --warp-fractal-type <None|DomainWarpProgressive|DomainWarpIndependent> (default None)\n");
    std::printf("  --warp-octaves <int> (default 3)\n");
    std::printf("  --warp-gain <float> (default 0.5)\n");
    std::printf("  --warp-lacunarity <float> (default 2.0)\n");
    std::printf("normalize:\n");
    std::printf("  --normalize <fixed|minmax> (default fixed)\n");
    std::printf("colormap:\n");
    std::printf("  --colormap <spec> (default grayscale)\n");
    std::printf("    preset: grayscale|terrain|viridis|magma|turbo|icefire\n");
    std::printf("    stops:  \"stops:0:#000000,0.5:#00ff00,1:#ffffff\"\n");
    std::printf("    file:   \"file:ramp.png\" (Nx1 or 1xN)\n");
    std::printf("    json:   \"json:ramp.json\" (format in README)\n");
    std::printf("output:\n");
    std::printf("  --out <path> (default out.png)\n");
    std::printf("  --format <png|jpg|jpeg|ppm> (optional; inferred from --out extension)\n");
    std::printf("  --csv <path.csv> (optional; dumps normalized t in [0,1])\n");
}

struct Cfg {
    int w = 512;
    int h = 512;
    int seed = 0;
    float freq = 0.01f;
    float z = 0.0f;
    std::string type = "Perlin";
    std::string rot3 = "None";
    bool tile = false;
    int tile_p = 0;
    std::string fract = "None";
    int oct = 5;
    float gain = 0.5f;
    float lac = 2.0f;
    float wstr = 0.0f;
    float pp = 2.0f;
    std::string cell_dist = "Euclidean";
    std::string cell_ret = "Distance";
    float cell_j = 1.0f;
    bool warp = false;
    std::string warp_type = "OpenSimplex2";
    float warp_amp = 1.0f;
    int warp_seed = 0;
    float warp_freq = 0.0f;
    std::string warp_rot3 = "";
    std::string warp_fract = "None";
    int warp_oct = 3;
    float warp_gain = 0.5f;
    float warp_lac = 2.0f;
    std::string norm = "fixed";
    std::string cmap = "grayscale";
    std::string out = "out.png";
    std::string fmt = "";
    std::string csv = "";
};

static FastNoiseLite::NoiseType nt(const std::string& s) {
    std::string t = lo(s);
    if (t == "opensimplex2") {
        return FastNoiseLite::NoiseType_OpenSimplex2;
    }
    if (t == "opensimplex2s") {
        return FastNoiseLite::NoiseType_OpenSimplex2S;
    }
    if (t == "simplex") {
        return FastNoiseLite::NoiseType_OpenSimplex2S;
    }
    if (t == "perlin") {
        return FastNoiseLite::NoiseType_Perlin;
    }
    if (t == "value") {
        return FastNoiseLite::NoiseType_Value;
    }
    if (t == "valuecubic") {
        return FastNoiseLite::NoiseType_ValueCubic;
    }
    if (t == "cellular") {
        return FastNoiseLite::NoiseType_Cellular;
    }
    throw std::runtime_error("bad --type: " + s);
}

static FastNoiseLite::RotationType3D rt3(const std::string& s) {
    std::string t = lo(s);
    if (t == "none") {
        return FastNoiseLite::RotationType3D_None;
    }
    if (t == "improvexyplanes") {
        return FastNoiseLite::RotationType3D_ImproveXYPlanes;
    }
    if (t == "improvexzplanes") {
        return FastNoiseLite::RotationType3D_ImproveXZPlanes;
    }
    throw std::runtime_error("bad --rotation3d: " + s);
}

static FastNoiseLite::FractalType ft(const std::string& s) {
    std::string t = lo(s);
    if (t == "none") {
        return FastNoiseLite::FractalType_None;
    }
    if (t == "fbm") {
        return FastNoiseLite::FractalType_FBm;
    }
    if (t == "rigid") {
        return FastNoiseLite::FractalType_Ridged;
    }
    if (t == "pingpong") {
        return FastNoiseLite::FractalType_PingPong;
    }
    throw std::runtime_error("bad --fractal-type: " + s);
}

static FastNoiseLite::CellularDistanceFunction cdf(const std::string& s) {
    std::string t = lo(s);
    if (t == "euclidean") {
        return FastNoiseLite::CellularDistanceFunction_Euclidean;
    }
    if (t == "euclideansq") {
        return FastNoiseLite::CellularDistanceFunction_EuclideanSq;
    }
    if (t == "manhattan") {
        return FastNoiseLite::CellularDistanceFunction_Manhattan;
    }
    if (t == "hybrid") {
        return FastNoiseLite::CellularDistanceFunction_Hybrid;
    }
    throw std::runtime_error("bad --cell-dist: " + s);
}

static FastNoiseLite::CellularReturnType crt(const std::string& s) {
    std::string t = lo(s);
    if (t == "cellvalue") {
        return FastNoiseLite::CellularReturnType_CellValue;
    }
    if (t == "distance") {
        return FastNoiseLite::CellularReturnType_Distance;
    }
    if (t == "distance2") {
        return FastNoiseLite::CellularReturnType_Distance2;
    }
    if (t == "distance2add") {
        return FastNoiseLite::CellularReturnType_Distance2Add;
    }
    if (t == "distance2sub") {
        return FastNoiseLite::CellularReturnType_Distance2Sub;
    }
    if (t == "distance2mul") {
        return FastNoiseLite::CellularReturnType_Distance2Mul;
    }
    if (t == "distance2div") {
        return FastNoiseLite::CellularReturnType_Distance2Div;
    }
    throw std::runtime_error("bad --cell-return: " + s);
}

static std::string warp_nt(const std::string& s) {
    std::string t = lo(s);
    if (t == "opensimplex2") {
        return "OpenSimplex2";
    }
    if (t == "opensimplex2reduced") {
        return "OpenSimplex2S";
    }
    if (t == "basicgrid") {
        return "Value";
    }
    throw std::runtime_error("bad --warp-type: " + s);
}

static std::string fmt_of(const Cfg& c) {
    if (!c.fmt.empty()) {
        return lo(c.fmt);
    }
    std::string e = ext_of(c.out);
    if (e == "png" || e == "jpg" || e == "jpeg" || e == "ppm") {
        return e;
    }
    return "png";
}

static float samp2(FastNoiseLite& n, float x, float y) {
    return n.GetNoise(x, y);
}

static float samp3(FastNoiseLite& n, float x, float y, float z) {
    return n.GetNoise(x, y, z);
}

static float tile4(FastNoiseLite& n, float x, float y, float p, float u, float v, bool use3, float z) {
    float a = use3 ? samp3(n, x, y, z) : samp2(n, x, y);
    float b = use3 ? samp3(n, x - p, y, z) : samp2(n, x - p, y);
    float c = use3 ? samp3(n, x, y - p, z) : samp2(n, x, y - p);
    float d = use3 ? samp3(n, x - p, y - p, z) : samp2(n, x - p, y - p);
    float ab = a + (b - a) * u;
    float cd = c + (d - c) * u;
    return ab + (cd - ab) * v;
}

static float val(FastNoiseLite& n, float x, float y, bool use3, float z) {
    return use3 ? samp3(n, x, y, z) : samp2(n, x, y);
}

static void warp_apply(FastNoiseLite& nx, FastNoiseLite& ny, float& x, float& y, const Cfg& c, bool tile, float p, float u, float v, bool use3, float z) {
    std::string t = lo(c.warp_fract);
    if (!tile) {
        if (t == "none") {
            float dx = val(nx, x, y, use3, z) * c.warp_amp;
            float dy = val(ny, x, y, use3, z) * c.warp_amp;
            x += dx;
            y += dy;
            return;
        }
        if (t == "domainwarpprogressive") {
            float f = 1.0f;
            float a = c.warp_amp;
            for (int i = 0; i < c.warp_oct; i++) {
                float dx = val(nx, x * f, y * f, use3, z) * a;
                float dy = val(ny, x * f, y * f, use3, z) * a;
                x += dx;
                y += dy;
                a *= c.warp_gain;
                f *= c.warp_lac;
            }
            return;
        }
        if (t == "domainwarpindependent") {
            float f = 1.0f;
            float a = c.warp_amp;
            float sx = 0.0f, sy = 0.0f;
            for (int i = 0; i < c.warp_oct; i++) {
                sx += val(nx, x * f, y * f, use3, z) * a;
                sy += val(ny, x * f, y * f, use3, z) * a;
                a *= c.warp_gain;
                f *= c.warp_lac;
            }
            x += sx;
            y += sy;
            return;
        }
        throw std::runtime_error("bad --warp-fractal-type: " + c.warp_fract);
    }
    if (t == "none") {
        float dx = tile4(nx, x, y, p, u, v, use3, z) * c.warp_amp;
        float dy = tile4(ny, x, y, p, u, v, use3, z) * c.warp_amp;
        x += dx;
        y += dy;
        return;
    }
    if (t == "domainwarpprogressive") {
        float f = 1.0f;
        float a = c.warp_amp;
        for (int i = 0; i < c.warp_oct; i++) {
            float px = p * f;
            float dx = tile4(nx, x * f, y * f, px, u, v, use3, z) * a;
            float dy = tile4(ny, x * f, y * f, px, u, v, use3, z) * a;
            x += dx;
            y += dy;
            a *= c.warp_gain;
            f *= c.warp_lac;
        }
        return;
    }
    if (t == "domainwarpindependent") {
        float f = 1.0f;
        float a = c.warp_amp;
        float sx = 0.0f, sy = 0.0f;
        for (int i = 0; i < c.warp_oct; i++) {
            float px = p * f;
            sx += tile4(nx, x * f, y * f, px, u, v, use3, z) * a;
            sy += tile4(ny, x * f, y * f, px, u, v, use3, z) * a;
            a *= c.warp_gain;
            f *= c.warp_lac;
        }
        x += sx;
        y += sy;
        return;
    }
    throw std::runtime_error("bad --warp-fractal-type: " + c.warp_fract);
}

static void write_csv(const std::string& path, int w, int h, const std::vector<float>& t) {
    std::ofstream f(path);
    if (!f) {
        throw std::runtime_error("failed to write csv: " + path);
    }
    f.setf(std::ios::fixed);
    f.precision(6);
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            if (x) {
                f << ",";
            }
            f << t[y * w + x];
        }
        f << "\n";
    }
}

static void write_ppm(const std::string& path, int w, int h, const std::vector<uint8_t>& img) {
    std::ofstream f(path, std::ios::binary);
    if (!f) {
        throw std::runtime_error("failed to write ppm: " + path);
    }
    f << "P6\n" << w << " " << h << "\n255\n";
    f.write((const char*) img.data(), (std::streamsize) img.size());
}

static Cfg cfg_from(const Args& a) {
    Cfg c;
    if (a.has("help") || a.has("h")) {
        help();
        std::exit(0);
    }
    if (a.has("width")) {
        if (!parse_i(a.get1("width", ""), c.w) || c.w <= 0) {
            throw std::runtime_error("bad --width");
        }
    }
    if (a.has("height")) {
        if (!parse_i(a.get1("height", ""), c.h) || c.h <= 0) {
            throw std::runtime_error("bad --height");
        }
    }
    if (a.has("seed")) {
        if (!parse_i(a.get1("seed", ""), c.seed)) {
            throw std::runtime_error("bad --seed");
        }
    }
    if (a.has("freq") || a.has("scale")) {
        std::string v = a.has("freq") ? a.get1("freq", "") : a.get1("scale", "");
        if (!parse_f(v, c.freq) || c.freq <= 0.0f) {
            throw std::runtime_error("bad --freq/--scale");
        }
    }
    if (a.has("z")) {
        if (!parse_f(a.get1("z", ""), c.z)) {
            throw std::runtime_error("bad --z");
        }
    }
    if (a.has("type")) {
        c.type = a.get1("type", c.type);
    }
    if (a.has("rotation3d")) {
        c.rot3 = a.get1("rotation3d", c.rot3);
    }
    if (a.has("tile")) {
        c.tile = true;
    }
    c.tile_p = c.w;
    if (a.has("tile-period")) {
        if (!parse_i(a.get1("tile-period", ""), c.tile_p) || c.tile_p <= 0) {
            throw std::runtime_error("bad --tile-period");
        }
    }
    if (a.has("fractal-type")) {
        c.fract = a.get1("fractal-type", c.fract);
    }
    if (a.has("octaves")) {
        if (!parse_i(a.get1("octaves", ""), c.oct) || c.oct < 1) {
            throw std::runtime_error("bad --octaves");
        }
    }
    if (a.has("gain")) {
        if (!parse_f(a.get1("gain", ""), c.gain)) {
            throw std::runtime_error("bad --gain");
        }
    }
    if (a.has("lacunarity")) {
        if (!parse_f(a.get1("lacunarity", ""), c.lac)) {
            throw std::runtime_error("bad --lacunarity");
        }
    }
    if (a.has("weighted-strength")) {
        if (!parse_f(a.get1("weighted-strength", ""), c.wstr)) {
            throw std::runtime_error("bad --weighted-strength");
        }
    }
    if (a.has("pingpong-strength")) {
        if (!parse_f(a.get1("pingpong-strength", ""), c.pp)) {
            throw std::runtime_error("bad --pingpong-strength");
        }
    }
    if (a.has("cell-dist")) {
        c.cell_dist = a.get1("cell-dist", c.cell_dist);
    }
    if (a.has("cell-return")) {
        c.cell_ret = a.get1("cell-return", c.cell_ret);
    }
    if (a.has("cell-jitter")) {
        if (!parse_f(a.get1("cell-jitter", ""), c.cell_j)) {
            throw std::runtime_error("bad --cell-jitter");
        }
    }
    if (a.has("warp")) {
        c.warp = true;
    }
    if (a.has("warp-type")) {
        c.warp_type = a.get1("warp-type", c.warp_type);
    }
    c.warp_seed = c.seed + 1;
    if (a.has("warp-seed")) {
        if (!parse_i(a.get1("warp-seed", ""), c.warp_seed)) {
            throw std::runtime_error("bad --warp-seed");
        }
    }
    c.warp_freq = c.freq;
    if (a.has("warp-freq")) {
        if (!parse_f(a.get1("warp-freq", ""), c.warp_freq) || c.warp_freq <= 0.0f) {
            throw std::runtime_error("bad --warp-freq");
        }
    }
    c.warp_rot3 = c.rot3;
    if (a.has("warp-rotation3d")) {
        c.warp_rot3 = a.get1("warp-rotation3d", c.warp_rot3);
    }
    if (a.has("warp-amp")) {
        if (!parse_f(a.get1("warp-amp", ""), c.warp_amp)) {
            throw std::runtime_error("bad --warp-amp");
        }
    }
    if (a.has("warp-fractal-type")) {
        c.warp_fract = a.get1("warp-fractal-type", c.warp_fract);
    }
    if (a.has("warp-octaves")) {
        if (!parse_i(a.get1("warp-octaves", ""), c.warp_oct) || c.warp_oct < 1) {
            throw std::runtime_error("bad --warp-octaves");
        }
    }
    if (a.has("warp-gain")) {
        if (!parse_f(a.get1("warp-gain", ""), c.warp_gain)) {
            throw std::runtime_error("bad --warp-gain");
        }
    }
    if (a.has("warp-lacunarity")) {
        if (!parse_f(a.get1("warp-lacunarity", ""), c.warp_lac)) {
            throw std::runtime_error("bad --warp-lacunarity");
        }
    }
    if (a.has("normalize")) {
        c.norm = a.get1("normalize", c.norm);
    }
    if (a.has("colormap")) {
        c.cmap = a.get1("colormap", c.cmap);
    }
    if (a.has("out")) {
        c.out = a.get1("out", c.out);
    }
    if (a.has("format")) {
        c.fmt = a.get1("format", c.fmt);
    }
    if (a.has("csv")) {
        c.csv = a.get1("csv", c.csv);
    }
    return c;
}

int main(int argc, char** argv) {
    try {
        Args a = Args::parse(argc, argv);
        Cfg c = cfg_from(a);
        FastNoiseLite n;
        n.SetSeed(c.seed);
        n.SetNoiseType(nt(c.type));
        n.SetRotationType3D(rt3(c.rot3));
        n.SetFrequency(c.freq);
        n.SetFractalType(ft(c.fract));
        n.SetFractalOctaves(c.oct);
        n.SetFractalGain(c.gain);
        n.SetFractalLacunarity(c.lac);
        n.SetFractalWeightedStrength(c.wstr);
        n.SetFractalPingPongStrength(c.pp);
        if (lo(c.type) == "cellular") {
            n.SetCellularDistanceFunction(cdf(c.cell_dist));
            n.SetCellularReturnType(crt(c.cell_ret));
            n.SetCellularJitter(c.cell_j);
        }
        FastNoiseLite wx, wy;
        if (c.warp) {
            std::string t = warp_nt(c.warp_type);
            wx.SetSeed(c.warp_seed);
            wy.SetSeed(c.warp_seed + 1);
            wx.SetNoiseType(nt(t));
            wy.SetNoiseType(nt(t));
            wx.SetRotationType3D(rt3(c.warp_rot3));
            wy.SetRotationType3D(rt3(c.warp_rot3));
            wx.SetFrequency(c.warp_freq);
            wy.SetFrequency(c.warp_freq);
        }
        bool use3 = c.z != 0.0f;
        std::vector<float> h((size_t) c.w * (size_t) c.h);
        float mn = 0.0f, mx = 0.0f;
        bool first = true;
        for (int y = 0; y < c.h; y++) {
            for (int x = 0; x < c.w; x++) {
                float v = 0.0f;
                if (!c.tile) {
                    float nx = (float) x;
                    float ny = (float) y;
                    if (c.warp) {
                        warp_apply(wx, wy, nx, ny, c, false, 0.0f, 0.0f, 0.0f, use3, c.z);
                    }
                    v = val(n, nx, ny, use3, c.z);
                } else {
                    int p = c.tile_p;
                    int xi = p <= 0 ? 0 : (x % p);
                    int yi = p <= 0 ? 0 : (y % p);
                    float u = (p <= 1) ? 0.0f : (float) xi / (float) (p - 1);
                    float v0 = (p <= 1) ? 0.0f : (float) yi / (float) (p - 1);
                    float per = (float) p;
                    float nx = u * per;
                    float ny = v0 * per;
                    if (c.warp) {
                        float tx = nx, ty = ny;
                        warp_apply(wx, wy, tx, ty, c, true, per, u, v0, use3, c.z);
                        nx = tx;
                        ny = ty;
                    }
                    v = tile4(n, nx, ny, per, u, v0, use3, c.z);
                }
                h[(size_t) y * (size_t) c.w + (size_t) x] = v;
                if (first) {
                    mn = mx = v;
                    first = false;
                } else {
                    if (v < mn) {
                        mn = v;
                    }
                    if (v > mx) {
                        mx = v;
                    }
                }
            }
        }
        std::vector<float> t((size_t) c.w * (size_t) c.h);
        std::string norm = lo(c.norm);
        if (norm != "fixed" && norm != "minmax") {
            throw std::runtime_error("bad --normalize: " + c.norm);
        }
        for (int i = 0; i < (int) t.size(); i++) {
            float v = h[i];
            if (norm == "fixed") {
                t[i] = clampv(v * 0.5f + 0.5f, 0.0f, 1.0f);
            } else {
                float d = mx - mn;
                t[i] = (d == 0.0f) ? 0.0f : clampv((v - mn) / d, 0.0f, 1.0f);
            }
        }
        if (!c.csv.empty()) {
            write_csv(c.csv, c.w, c.h, t);
        }
        Colormap m = Colormap::parse(c.cmap);
        std::vector<uint8_t> img((size_t) c.w * (size_t) c.h * 3u);
        for (int i = 0; i < (int) t.size(); i++) {
            RGB c0 = m.at(t[i]);
            img[(size_t) i * 3u + 0u] = c0.r;
            img[(size_t) i * 3u + 1u] = c0.g;
            img[(size_t) i * 3u + 2u] = c0.b;
        }
        std::string f = fmt_of(c);
        if (f == "ppm") {
            write_ppm(c.out, c.w, c.h, img);
        } else if (f == "png") {
            if (!stbi_write_png(c.out.c_str(), c.w, c.h, 3, img.data(), c.w * 3)) {
                throw std::runtime_error("png write failed: " + c.out);
            }
        } else if (f == "jpg" || f == "jpeg") {
            if (!stbi_write_jpg(c.out.c_str(), c.w, c.h, 3, img.data(), 95)) {
                throw std::runtime_error("jpg write failed: " + c.out);
            }
        } else {
            throw std::runtime_error("bad format: " + f);
        }
        return 0;
    } catch (const std::exception& e) {
        std::fprintf(stderr, "error: %s\n", e.what());
        std::fprintf(stderr, "run with --help for usage\n");
        return 1;
    }
}