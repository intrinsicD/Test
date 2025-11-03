#pragma once

namespace engine::rendering
{
    struct RuntimeSubmissionContext;
} // namespace engine::rendering

namespace engine::runtime
{
    class RuntimeHost;

    /// Submit a compiled frame graph to \p host using the provided submission context.
    void submit_render_graph(RuntimeHost& host, rendering::RuntimeSubmissionContext& context);
}

