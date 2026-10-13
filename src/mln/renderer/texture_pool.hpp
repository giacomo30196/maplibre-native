#pragma once

#include <mln/tile/tile_id.hpp>
#include <mln/renderer/render_target.hpp>
#include <optional>
#include <set>
#include <vector>

namespace mln {
class TexturePool {
public:
    TexturePool(uint32_t tilesize);
    ~TexturePool();

    std::shared_ptr<RenderTarget> getRenderTarget(const UnwrappedTileID& id) const;
    std::shared_ptr<RenderTarget> getRenderTargetAncestorOrDescendant(
        const UnwrappedTileID& id, std::optional<UnwrappedTileID>& terrainTileID) const;
    void createRenderTarget(gfx::Context& context, const UnwrappedTileID& id, const Color& backgroundColor);

    /// Remove render targets for tiles that are no longer part of the given set. Some are kept as
    /// spares for tiles that join later; an empty set (terrain off) releases the spares too.
    void removeStaleRenderTargets(const std::set<UnwrappedTileID>& currentTiles);

    template <typename Func /* void(std::shared_ptr<RenderTarget>&) */>
    void visitRenderTargets(Func f) {
        for (auto& pair : renderTargets) {
            if (pair.second) {
                f(pair.second);
            }
        }
    }

    template <typename Func /* void(std::shared_ptr<RenderTarget>&) */>
    void visitRenderTargets(Func f) const {
        for (const auto& pair : renderTargets) {
            if (pair.second) {
                f(pair.second);
            }
        }
    }

private:
    uint32_t tileSize;

    std::map<UnwrappedTileID, std::shared_ptr<RenderTarget>> renderTargets;

    // Targets whose tile left the cover, handed to the next tiles that join it. Riding at
    // 50 km/h tiles leave and join the cover several times a second (the same tile often
    // comes back), and a target created for each, deleted when it left, grew GPU memory
    // without bound: the Mali G72 driver kept what the deleted framebuffers and depth
    // buffers held, 0.9 to 1.6 GB in three minutes. Up to a whole cover's worth (the mesh
    // cap) is kept, since a turn or a zoom step replaces most of it at once: with 16 spares
    // such bursts still created targets.
    std::vector<std::shared_ptr<RenderTarget>> spareRenderTargets;
    static constexpr std::size_t maxSpareRenderTargets = 64;
};

} // namespace mln
