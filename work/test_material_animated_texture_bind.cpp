#include "material_object_store_candidate.h"
#include "material_animated_texture_bind_proxy_candidate.h"
#include "phong_detail_draw_binding_candidate.h"
#include "phong_ambient_pixel_fixture.h"
#include "vtf_block_decode.h"
#include <iostream>

using Store = MaterialObjectStoreCandidate;
using Node = SourceMaterialPatchNodeCandidate;
using Reference = MaterialTextureReferenceCandidate;

static void check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

static const MaterialTypedDispatchCandidate& parameter(
    const MaterialObjectParameterStateCandidate& state, const char* name) {
    for (const auto& value : state.material.parameters.values)
        if (value.name == name) return value;
    throw std::runtime_error("expected draw parameter");
}

static uint32_t bits(float value) {
    uint32_t result;
    std::memcpy(&result, &value, sizeof(result));
    return result;
}

static Reference fixtureTexture(const char* name, uint16_t frames) {
    constexpr size_t pixels = 16;
    std::vector<uint8_t> bytes(80 + 4 * pixels * frames, 0);
    const auto put = [&](size_t offset, auto value) { std::memcpy(bytes.data() + offset, &value, sizeof(value)); };
    put(0, uint32_t{0x00465456});
    put(4, uint32_t{7});
    put(8, uint32_t{2});
    put(12, uint32_t{80});
    put(16, uint16_t{4});
    put(18, uint16_t{4});
    put(24, frames);
    put(63, uint16_t{1});
    bytes[56] = 1;
    for (uint16_t frame = 0; frame < frames; ++frame) {
        for (size_t pixel = 0; pixel < pixels; ++pixel) {
            const auto offset = 80 + 4 * (frame * pixels + pixel);
            bytes[offset] = uint8_t(50 + 60 * frame + 4 * pixel);
            bytes[offset + 1] = uint8_t(220 - 30 * frame - 5 * pixel);
            bytes[offset + 2] = uint8_t(80 + 45 * frame + 3 * pixel);
            bytes[offset + 3] = 255;
        }
    }
    return Reference::ordinary(MaterialTextureResourceCandidate::fromVtf(name, bytes));
}

static Store::Id addMaterial(Store& store, bool noWrap, bool explicitRate) {
    auto root = std::make_unique<Node>();
    root->key = "VertexLitGeneric";
    root->children = {{"$detail", 1, "fixture/original"}, {"$detailframe", 2, "", 26},
                      {"$detailblendmode", 2, "", 6}, {"$detailblendfactor", 3, "", bits(0.75f)},
                      {"$detailscale", 3, "", bits(2.0f)}};
    Node declaration{"AnimatedTexture"};
    declaration.children = {{"animatedTextureVar", 1, "$detail"},
                            {"animatedTextureFrameNumVar", 1, "$detailframe"}};
    if (noWrap) declaration.children.push_back({"animationNoWrap", 1, "1"});
    if (explicitRate) declaration.children.push_back({"animatedTextureFrameRate", 1, "30"});
    Node proxies{"Proxies"};
    proxies.children.push_back(std::move(declaration));
    root->children.push_back(std::move(proxies));
    MaterialObjectParameterOptionsCandidate options;
    options.proxyFactory = std::make_shared<MaterialPinnedAnimatedDetailProxyFactoryCandidate>();
    Store::Id id = 0;
    check(store.createMaterial("fixture/animated/" + std::to_string(store.size()), root, id) &&
          store.initializeParameters(id, options), "initialize fixture material");
    return id;
}

static MaterialSessionOverrideInputCandidate textureOverride(const char* name) {
    MaterialSessionOverrideInputCandidate row;
    row.name = "$detail";
    row.typeFlags = 5;
    row.textureName = name;
    return row;
}

static MaterialProxyEvaluationInputsCandidate clockAt(float time) {
    MaterialProxyEvaluationInputsCandidate input;
    input.currentTimeBits = bits(time);
    input.frameDurationBits = bits(0.025f);
    return input;
}

int main(int argc, char** argv) {
    try {
        static_assert(sizeof(void*) == 8);
        check(argc == 1 || argc == 2, "optional fresh GPU input directory");
        std::ofstream queries, textures;
        if (argc == 2) {
            const std::filesystem::path directory(argv[1]);
            check(std::filesystem::create_directory(directory), "fresh GPU input directory");
            queries.open(directory / "queries.bin", std::ios::binary);
            textures.open(directory / "textures.bin", std::ios::binary);
            check(queries.good() && textures.good(), "open GPU input files");
        }
        Store store;
        const auto loop = addMaterial(store, false, true);
        const auto noWrap = addMaterial(store, true, true);
        const auto defaultRate = addMaterial(store, false, false);
        const auto original = fixtureTexture("fixture/original", 2);
        const auto replacement = fixtureTexture("fixture/replacement", 3);
        const MaterialTextureResolverCandidate lookup = [&](const auto& name) -> std::optional<Reference> {
            if (name == original.resource()->name()) return original;
            if (name == replacement.resource()->name()) return replacement;
            return {};
        };
        const auto override = textureOverride("fixture/replacement");
        MaterialSessionOverrideInputCandidate frameOverride;
        frameOverride.name = "$detailframe";
        frameOverride.typeFlags = 2;
        frameOverride.words[0] = 23;
        MaterialSessionOverrideInputCandidate zeroBlend;
        zeroBlend.name = "$detailblendfactor";
        zeroBlend.typeFlags = 3;
        zeroBlend.words[0] = bits(0);
        const std::vector<Store::SessionEntry> entries{{loop, {override, frameOverride}},
            {noWrap, {override}}, {defaultRate, {override, zeroBlend}}};
        const std::vector<Store::SessionDrawStep> steps{{loop, clockAt(0.08f)}, {loop, clockAt(0.101f)},
            {noWrap, clockAt(0.101f)}, {noWrap, clockAt(0.151f)}, {defaultRate, clockAt(0.101f)}};
        const uint32_t expected[] = {2, 0, 2, 1, 1};
        unsigned observations = 0;
        const auto observe = [&](const Store::SessionDrawView& draw) {
            try {
                ++observations;
                check(draw.steps.size() == std::size(expected), "draw snapshot count");
                for (size_t index = 0; index < draw.steps.size(); ++index) {
                    const auto& step = draw.steps[index];
                    const auto& field = parameter(*step.parameters, "$detailframe");
                    const auto& texture = parameter(*step.parameters, "$detail").texture;
                    check((field.value.numeric.typeAndMetadata & 15u) == 4 &&
                          field.value.numeric.integer == expected[index] && texture == replacement,
                          "per-draw frame uses the override texture's frame count");
                    check(step.writes->size() == 1 && step.writes->at(0).arguments[0] == expected[index],
                          "exact integer proxy write");
                    const auto binding = resolvePhongDetailDrawBindingCandidate(step.parameters->material.parameters.values);
                    check(binding && binding->texture == replacement && binding->frame == expected[index] &&
                          binding->blendFactor == (index == 4 ? 0.0f : 0.75f) &&
                          binding->uvRows == std::array<float, 8>{2, 0, 0, 0, 0, 2, 0, 0},
                          "typed Phong detail binding");
                    const auto image = decodeVtfMipCandidate(binding->texture.resource()->bytes(), 0, binding->frame);
                    check(image.rgba[0] == 50 + 60 * expected[index], "selected VTF frame reaches CPU texture decode");
                    if (argc == 2) {
                        PhongAmbientPixelQuery query{};
                        query.normalSample = {0.5f, 0.5f, 1, 0};
                        query.normal = {0, 0, 1, 0};
                        std::copy_n(binding->uvRows.begin(), 4, query.tangent.begin());
                        std::copy_n(binding->uvRows.begin() + 4, 4, query.binormal.begin());
                        query.eye = {0, 0, 1, 0};
                        query.fog = {0, 0, 1, 0};
                        query.lightScale[0] = 1;
                        query.controls[3] = binding->blendFactor;
                        queries.write(reinterpret_cast<const char*>(&query), sizeof(query));
                        for (const auto byte : image.rgba) {
                            const float component = float(byte) / 255.0f;
                            textures.write(reinterpret_cast<const char*>(&component), sizeof(component));
                        }
                    }
                }
                return true;
            } catch (const std::exception& error) {
                std::cerr << "draw observer: " << error.what() << '\n';
                return false;
            }
        };
        auto missingClock = steps;
        missingClock.back().inputs.frameDurationBits.reset();
        const auto* before = store.get(loop)->parameterState.get();
        check(!store.withSessionDrawSteps(entries, missingClock, observe, 0, {}, lookup) &&
              observations == 0 && store.get(loop)->parameterState.get() == before,
              "missing duration refuses and rolls back entire draw group");
        check(!store.withSessionDrawSteps({}, {{loop, clockAt(0.101f)}}, observe) &&
              observations == 0, "unresolved texture refuses instead of guessing frame count");
        check(store.withSessionDrawSteps(entries, steps, observe, 0, {}, lookup) && observations == 1,
              "publish five controlled draw snapshots");
        for (auto id : {loop, noWrap, defaultRate}) {
            const auto& state = *store.get(id)->parameterState;
            check(parameter(state, "$detail").texture == original, "texture override restored");
            check(parameter(state, "$detailframe").value.numeric.integer ==
                      (id == loop ? 26u : 1u), "restore overridden frame and preserve unrelated proxy frame");
        }
        if (argc == 2) {
            queries.flush();
            textures.flush();
            check(queries.good() && textures.good(), "write complete GPU fixture");
        }
        std::cout << "PASS animated_texture_draws=5 clock_refusal=1 texture_refusal=1 restored_materials=3\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
