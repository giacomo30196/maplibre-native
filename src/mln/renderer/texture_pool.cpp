#include <mln/renderer/texture_pool.hpp>
#include <mln/gfx/context.hpp>
#include <mln/util/logging.hpp>

namespace mln {
TexturePool::TexturePool(uint32_t tilesize)
    : tileSize(tilesize) {}

TexturePool::~TexturePool() {}

void TexturePool::createRenderTarget(gfx::Context& context, const UnwrappedTileID& id, const Color& backgroundColor) {
    // Keep an existing render target; recreating it every frame churns GL
    // texture memory and invalidates the texture bound to the terrain drawable
    if (auto it = renderTargets.find(id); it != renderTargets.end() && it->second) {
        it->second->setClearColor(backgroundColor);
        return;
    }
    // A spare only once nothing else holds it or its texture: the orchestrator's target list and
    // the terrain drawable of the tile it last served let go of it the frame after it left.
    std::shared_ptr<RenderTarget> renderTarget;
    while (!renderTarget && !spareRenderTargets.empty()) {
        auto spare = std::move(spareRenderTargets.back());
        spareRenderTargets.pop_back();
        if (spare.use_count() == 1 && spare->getTexture().use_count() == 1) {
            renderTarget = std::move(spare);
            renderTarget->resetDrapeContent();
        }
    }
    if (!renderTarget) {
        // Drape targets render several overlapping draped tiles (a parent standing in for
        // missing children beside already-loaded children); the stencil clips them against
        // each other, as maplibre-gl-js does for its render-to-texture framebuffer.
        renderTarget = context.createRenderTarget(
            {tileSize, tileSize}, gfx::TextureChannelDataType::UnsignedByte, /*stencil=*/true);
    }
    renderTarget->setClearColor(backgroundColor);
    renderTarget->setDrapeTileID(id);
    renderTargets[id] = std::move(renderTarget);
}

void TexturePool::removeStaleRenderTargets(const std::set<UnwrappedTileID>& currentTiles) {
    for (auto it = renderTargets.begin(); it != renderTargets.end();) {
        if (!currentTiles.contains(it->first)) {
            if (!currentTiles.empty() && spareRenderTargets.size() < maxSpareRenderTargets) {
                spareRenderTargets.push_back(std::move(it->second));
            }
            it = renderTargets.erase(it);
        } else {
            ++it;
        }
    }
    if (currentTiles.empty()) {
        spareRenderTargets.clear();
    }
}

std::shared_ptr<RenderTarget> TexturePool::getRenderTarget(const UnwrappedTileID& id) const {
    return renderTargets.contains(id) ? renderTargets.at(id) : nullptr;
}

std::shared_ptr<RenderTarget> TexturePool::getRenderTargetAncestorOrDescendant(
    const UnwrappedTileID& id, std::optional<UnwrappedTileID>& terrainTileID) const {
    terrainTileID = std::nullopt;
    std::shared_ptr<RenderTarget> bestRenderTarget;
    for (const auto& [tileID, renderTarget] : renderTargets) {
        if (tileID == id || tileID.isChildOf(id) || id.isChildOf(tileID)) {
            if (!terrainTileID || tileID.canonical.z > terrainTileID->canonical.z) {
                terrainTileID = tileID;
                bestRenderTarget = renderTarget;
            }
        }
    }
    return bestRenderTarget;
}
} // namespace mln
