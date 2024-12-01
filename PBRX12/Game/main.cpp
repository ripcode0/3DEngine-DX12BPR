#include "renderer.h"
// lets pop a window and use D3D12 to clear to a jade colored screen











int main()
{
	GWindow win;
	GEventResponder msgs;
	GDirectX12Surface d3d12;
	if (+win.Create(0, 0, 800, 600, GWindowStyle::WINDOWEDBORDERED))
	{
		// TODO: Part 1a 
		float clr[] = { 0, 168 / 255.0f, 107 / 255.0f, 1 }; // start with a jade color
		msgs.Create([&](const GW::GEvent& e) {
			GW::SYSTEM::GWindow::Events q;
			if (+e.Read(q) && q == GWindow::Events::RESIZE)
				clr[0] += 0.01f; // move towards a orange as they resize
			});
		win.Register(msgs);
		if (+d3d12.Create(win, GW::GRAPHICS::DEPTH_BUFFER_SUPPORT))
		{
			Renderer renderer(win, d3d12); // init
			while (+win.ProcessWindowEvents())
			{
				if (+d3d12.StartFrame())
				{
					ID3D12GraphicsCommandList* cmd;
					D3D12_CPU_DESCRIPTOR_HANDLE rtv;
					D3D12_CPU_DESCRIPTOR_HANDLE dsv;
					if (+d3d12.GetCommandList((void**)&cmd) &&
						+d3d12.GetCurrentRenderTargetView((void**)&rtv) &&
						+d3d12.GetDepthStencilView((void**)&dsv))
					{
						cmd->ClearRenderTargetView(rtv, clr, 0, nullptr);
						cmd->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH, 1, 0, 0, nullptr);
						renderer.Render(); // draw
						d3d12.EndFrame(false);
						cmd->Release();
					}
				}
			}// clean-up when renderer falls off stack
		}
	}
	return 0; // that's all folks
}