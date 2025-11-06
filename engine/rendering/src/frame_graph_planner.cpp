#include "engine/rendering/frame_graph_planner.hpp"

#include <algorithm>
#include <limits>
#include <stdexcept>
#include <unordered_map>
#include <utility>

namespace
{
    constexpr std::size_t kInvalidIndex = std::numeric_limits<std::size_t>::max();

    struct ResourceSignature
    {
        engine::rendering::ResourceKind kind;
        engine::rendering::ResourceFormat format;
        engine::rendering::ResourceDimension dimension;
        std::uint32_t width;
        std::uint32_t height;
        std::uint32_t depth;
        std::uint32_t array_layers;
        std::uint32_t mip_levels;
        engine::rendering::ResourceSampleCount sample_count;

        friend bool operator==(const ResourceSignature& lhs, const ResourceSignature& rhs) noexcept
        {
            return lhs.kind == rhs.kind && lhs.format == rhs.format && lhs.dimension == rhs.dimension &&
                lhs.width == rhs.width && lhs.height == rhs.height && lhs.depth == rhs.depth &&
                lhs.array_layers == rhs.array_layers && lhs.mip_levels == rhs.mip_levels &&
                lhs.sample_count == rhs.sample_count;
        }
    };

    struct ResourceSignatureHash
    {
        [[nodiscard]] std::size_t operator()(const ResourceSignature& signature) const noexcept
        {
            std::size_t seed = 0;
            const auto combine = [&seed](std::size_t value)
            {
                seed ^= value + 0x9e3779b97f4a7c15ULL + (seed << 6U) + (seed >> 2U);
            };

            combine(static_cast<std::size_t>(signature.kind));
            combine(static_cast<std::size_t>(signature.format));
            combine(static_cast<std::size_t>(signature.dimension));
            combine(signature.width);
            combine(signature.height);
            combine(signature.depth);
            combine(signature.array_layers);
            combine(signature.mip_levels);
            combine(static_cast<std::size_t>(signature.sample_count));
            return seed;
        }
    };

    [[nodiscard]] ResourceSignature make_signature(const engine::rendering::ResourceDesc& desc) noexcept
    {
        return ResourceSignature{desc.kind,
            desc.format,
            desc.dimension,
            desc.width,
            desc.height,
            desc.depth,
            desc.array_layers,
            desc.mip_levels,
            desc.sample_count};
    }
}

namespace engine::rendering
{
    namespace
    {
        struct ResourceInfo
        {
            std::string name;
            ResourceDesc descriptor;
            bool external{false};
            bool transient{false};
            std::size_t creating_pass{kInvalidIndex};
            std::size_t writing_pass{kInvalidIndex};
            std::vector<std::size_t> readers;
            std::size_t first_use{kInvalidIndex};
            std::size_t last_use{kInvalidIndex};
            std::size_t alias{kInvalidIndex};
        };

        struct PassInfo
        {
            NodeDescriptor descriptor;
            std::unique_ptr<INode> node;
            std::vector<std::size_t> creates;
            std::vector<std::size_t> reads;
            std::vector<std::size_t> writes;
        };
    }

    FrameGraphPlanner::Plan::Plan() = default;
    FrameGraphPlanner::Plan::Plan(Plan&&) noexcept = default;
    FrameGraphPlanner::Plan& FrameGraphPlanner::Plan::operator=(Plan&&) noexcept = default;
    FrameGraphPlanner::Plan::~Plan() = default;

    const std::vector<FrameGraphPlanner::PlannedPass>& FrameGraphPlanner::Plan::passes() const noexcept
    {
        return passes_;
    }

    const std::vector<FrameGraphPlanner::PlannedResource>& FrameGraphPlanner::Plan::resources() const noexcept
    {
        return resources_;
    }

    std::optional<std::size_t> FrameGraphPlanner::Plan::find_resource(std::string_view name) const
    {
        const auto it = resource_lookup_.find(name);
        if (it == resource_lookup_.end())
        {
            return std::nullopt;
        }
        return it->second;
    }

    FrameGraphPlanner::FrameGraphPlanner(const FrameGraphNodeRegistry& registry) noexcept
        : registry_{&registry}
    {
    }

    FrameGraphPlanner::Plan FrameGraphPlanner::plan(const PlanRequest& request) const
    {
        if (!registry_)
        {
            throw std::logic_error("FrameGraphPlanner constructed without registry");
        }

        Plan result{};

        if (request.nodes.empty())
        {
            // No passes requested; still track external resources if provided.
            result.resources_.reserve(request.external_resources.size());
            for (const auto& external : request.external_resources)
            {
                if (external.name.empty())
                {
                    throw std::invalid_argument("External resource name must not be empty");
                }

                PlannedResource resource{};
                resource.name = external.name;
                resource.descriptor = external;
                resource.descriptor.transient = false;
                resource.external = true;
                resource.transient = false;
                result.resource_lookup_.emplace(resource.name, result.resources_.size());
                result.resources_.push_back(std::move(resource));
            }
            return result;
        }

        std::vector<ResourceInfo> resources;
        resources.reserve(request.nodes.size() * 2U + request.external_resources.size());
        std::unordered_map<std::string, std::size_t, TransparentStringHash, std::equal_to<>> resource_lookup;

        const auto add_resource = [&](ResourceDesc descriptor, bool external, std::size_t creating_pass) -> std::size_t
        {
            if (descriptor.name.empty())
            {
                throw std::invalid_argument("Resource descriptors must declare a name");
            }

            if (resource_lookup.find(descriptor.name) != resource_lookup.end())
            {
                throw std::runtime_error("Duplicate resource declaration: " + descriptor.name);
            }

            ResourceInfo info{};
            info.name = descriptor.name;
            info.descriptor = std::move(descriptor);
            info.external = external;
            info.transient = info.descriptor.transient && !external;
            info.creating_pass = creating_pass;
            if (creating_pass != kInvalidIndex)
            {
                info.writing_pass = creating_pass;
            }

            const auto index = resources.size();
            resources.push_back(std::move(info));
            resource_lookup.emplace(resources.back().name, index);
            return index;
        };

        for (const auto& external : request.external_resources)
        {
            ResourceDesc descriptor = external;
            descriptor.transient = false;
            add_resource(std::move(descriptor), true, kInvalidIndex);
        }

        std::vector<PassInfo> passes;
        passes.reserve(request.nodes.size());

        for (std::size_t pass_index = 0; pass_index < request.nodes.size(); ++pass_index)
        {
            const auto& node_id = request.nodes[pass_index];
            if (node_id.empty())
            {
                throw std::invalid_argument("Node id must not be empty");
            }

            std::unique_ptr<INode> node = registry_->create(node_id);
            if (!node)
            {
                throw std::runtime_error("Node factory returned null instance for id: " + node_id);
            }

            PassInfo pass{};
            pass.descriptor = node->Reflect();
            if (pass.descriptor.id != node_id)
            {
                throw std::runtime_error("Node descriptor id mismatch for node: " + node_id);
            }

            pass.node = std::move(node);

            for (const auto& created : pass.descriptor.creates)
            {
                const auto resource_index = add_resource(created, false, pass_index);
                pass.creates.push_back(resource_index);
            }

            for (const auto& write : pass.descriptor.writes)
            {
                const auto it = resource_lookup.find(write.name);
                if (it == resource_lookup.end())
                {
                    throw std::runtime_error("Resource " + write.name + " written by " + node_id + " has no producer");
                }

                auto& resource = resources[it->second];
                if (resource.writing_pass != kInvalidIndex && resource.writing_pass != pass_index)
                {
                    throw std::runtime_error("Multiple nodes write resource " + write.name);
                }

                resource.writing_pass = pass_index;
                pass.writes.push_back(it->second);
            }

            for (const auto& read : pass.descriptor.reads)
            {
                const auto it = resource_lookup.find(read.name);
                if (it == resource_lookup.end())
                {
                    throw std::runtime_error("Resource " + read.name + " read by " + node_id + " has no producer");
                }

                auto& resource = resources[it->second];
                resource.readers.push_back(pass_index);
                pass.reads.push_back(it->second);
            }

            passes.push_back(std::move(pass));
        }

        for (const auto& resource : resources)
        {
            if (!resource.external && resource.writing_pass == kInvalidIndex)
            {
                throw std::runtime_error("Resource " + resource.name + " is missing a producer");
            }
        }

        const std::size_t pass_count = passes.size();
        std::vector<std::vector<std::size_t>> adjacency(pass_count);
        std::vector<std::size_t> indegree(pass_count, 0U);

        for (const auto& resource : resources)
        {
            if (resource.writing_pass == kInvalidIndex)
            {
                continue;
            }

            std::vector<std::size_t> unique_readers;
            unique_readers.reserve(resource.readers.size());

            for (const auto reader_index : resource.readers)
            {
                if (reader_index == resource.writing_pass)
                {
                    continue;
                }

                if (std::find(unique_readers.begin(), unique_readers.end(), reader_index) == unique_readers.end())
                {
                    unique_readers.push_back(reader_index);
                }
            }

            for (const auto reader_index : unique_readers)
            {
                adjacency[resource.writing_pass].push_back(reader_index);
                ++indegree[reader_index];
            }
        }

        std::vector<std::size_t> ready;
        ready.reserve(pass_count);
        for (std::size_t index = 0; index < pass_count; ++index)
        {
            if (indegree[index] == 0U)
            {
                ready.push_back(index);
            }
        }
        std::sort(ready.begin(), ready.end());

        std::vector<std::size_t> order;
        order.reserve(pass_count);

        while (!ready.empty())
        {
            const auto current = ready.front();
            ready.erase(ready.begin());
            order.push_back(current);

            for (const auto neighbour : adjacency[current])
            {
                auto& degree = indegree[neighbour];
                if (degree == 0U)
                {
                    continue;
                }

                --degree;
                if (degree == 0U)
                {
                    const auto insert_pos = std::lower_bound(ready.begin(), ready.end(), neighbour);
                    ready.insert(insert_pos, neighbour);
                }
            }
        }

        if (order.size() != pass_count)
        {
            throw std::runtime_error("Detected cycle while planning frame-graph nodes");
        }

        std::vector<std::size_t> schedule_position(pass_count, kInvalidIndex);
        for (std::size_t position = 0; position < order.size(); ++position)
        {
            schedule_position[order[position]] = position;
        }

        for (auto& resource : resources)
        {
            resource.first_use = kInvalidIndex;
            resource.last_use = kInvalidIndex;
        }

        for (std::size_t position = 0; position < order.size(); ++position)
        {
            const auto pass_index = order[position];
            const auto& pass = passes[pass_index];

            const auto update_use = [&](std::size_t resource_index)
            {
                auto& resource = resources[resource_index];
                if (resource.first_use == kInvalidIndex)
                {
                    resource.first_use = position;
                }
                else
                {
                    resource.first_use = std::min(resource.first_use, position);
                }

                if (resource.last_use == kInvalidIndex)
                {
                    resource.last_use = position;
                }
                else
                {
                    resource.last_use = std::max(resource.last_use, position);
                }
            };

            for (const auto resource_index : pass.creates)
            {
                update_use(resource_index);
            }
            for (const auto resource_index : pass.reads)
            {
                update_use(resource_index);
            }
            for (const auto resource_index : pass.writes)
            {
                update_use(resource_index);
            }
        }

        struct AliasPool
        {
            std::vector<std::size_t> free_aliases;
            std::vector<std::pair<std::size_t, std::size_t>> active;
        };

        std::unordered_map<ResourceSignature, AliasPool, ResourceSignatureHash> alias_pools;
        std::size_t next_alias = 0;

        struct AliasCandidate
        {
            std::size_t resource_index;
            std::size_t first;
            std::size_t last;
        };

        std::vector<AliasCandidate> candidates;
        for (std::size_t index = 0; index < resources.size(); ++index)
        {
            const auto& resource = resources[index];
            if (!resource.transient)
            {
                continue;
            }

            if (resource.first_use == kInvalidIndex || resource.last_use == kInvalidIndex)
            {
                continue;
            }

            candidates.push_back(AliasCandidate{index, resource.first_use, resource.last_use});
        }

        std::sort(candidates.begin(), candidates.end(),
            [](const AliasCandidate& lhs, const AliasCandidate& rhs)
            {
                if (lhs.first != rhs.first)
                {
                    return lhs.first < rhs.first;
                }
                return lhs.last < rhs.last;
            });

        for (const auto& candidate : candidates)
        {
            auto& resource = resources[candidate.resource_index];
            auto& pool = alias_pools[make_signature(resource.descriptor)];

            const auto release_finished = [&](std::size_t current_first)
            {
                auto it = std::remove_if(pool.active.begin(), pool.active.end(),
                    [&](const std::pair<std::size_t, std::size_t>& active)
                    {
                        if (active.first < current_first)
                        {
                            pool.free_aliases.push_back(active.second);
                            return true;
                        }
                        return false;
                    });
                pool.active.erase(it, pool.active.end());
            };

            release_finished(candidate.first);

            std::size_t alias = kInvalidIndex;
            if (!pool.free_aliases.empty())
            {
                alias = pool.free_aliases.back();
                pool.free_aliases.pop_back();
            }
            else
            {
                alias = next_alias++;
            }

            pool.active.emplace_back(candidate.last, alias);
            resource.alias = alias;
        }

        result.passes_.reserve(pass_count);
        for (const auto pass_index : order)
        {
            PlannedPass planned{};
            planned.id = passes[pass_index].descriptor.id;
            planned.descriptor = passes[pass_index].descriptor;
            planned.node = std::move(passes[pass_index].node);
            planned.queue = QueueType::Graphics;
            planned.creates = passes[pass_index].creates;
            planned.reads = passes[pass_index].reads;
            planned.writes = passes[pass_index].writes;
            result.passes_.push_back(std::move(planned));
        }

        result.resources_.reserve(resources.size());
        for (std::size_t index = 0; index < resources.size(); ++index)
        {
            PlannedResource planned{};
            planned.name = resources[index].name;
            planned.descriptor = resources[index].descriptor;
            planned.external = resources[index].external;
            planned.transient = resources[index].transient;
            planned.alias = resources[index].alias;
            planned.first_use = resources[index].first_use;
            planned.last_use = resources[index].last_use;

            result.resource_lookup_.emplace(planned.name, index);
            result.resources_.push_back(std::move(planned));
        }

        return result;
    }
}
