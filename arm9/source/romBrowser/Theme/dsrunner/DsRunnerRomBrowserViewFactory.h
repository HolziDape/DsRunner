#pragma once
#include "romBrowser/Theme/IRomBrowserViewFactory.h"
#include "romBrowser/viewModels/RomBrowserViewModel.h"
#include "romBrowser/Theme/Material/CarouselRecyclerView.h"
#include "romBrowser/Theme/Material/MaterialCoverFlowFileRecyclerAdapter.h"
#include "DsRunnerIconGridItemView.h"
#include "DsRunnerBannerListItemView.h"
#include "DsRunnerAppBarView.h"
#include "DsRunnerFileInfoCardView.h"

class MaterialColorScheme;
class IFontRepository;

class DsRunnerRomBrowserViewFactory : public IRomBrowserViewFactory
{
public:
    DsRunnerRomBrowserViewFactory(const MaterialColorScheme* colorScheme,
        const IFontRepository* fontRepository);

    SharedPtr<IconGridItemView> CreateIconGridItemView(std::unique_ptr<IRomBrowserItemViewModel> viewModel) const override
    {
        return DsRunnerIconGridItemView::CreateShared(std::move(viewModel), _colorScheme);
    }

    IconGridItemView::VramToken UploadIconGridItemViewGraphics(const VramContext& vramContext) const override
    {
        return DsRunnerIconGridItemView::UploadGraphics(vramContext);
    }

    SharedPtr<BannerListItemView> CreateBannerListItemView(std::unique_ptr<IRomBrowserItemViewModel> viewModel,
        VBlankTextureLoader* vblankTextureLoader) const override
    {
        return DsRunnerBannerListItemView::CreateShared(std::move(viewModel), _colorScheme, _fontRepository);
    }

    BannerListItemView::VramToken UploadBannerListItemViewGraphics(const VramContext& vramContext) const override
    {
        return DsRunnerBannerListItemView::UploadGraphics(vramContext);
    }

    SharedPtr<AppBarView> CreateAppBarView(int x, int y, AppBarView::Orientation orientation,
        int startButtonCount, int endButtonCount) const override
    {
        return DsRunnerAppBarView::CreateShared(x, y, orientation, startButtonCount, endButtonCount, _colorScheme, _fontRepository);
    }

    SharedPtr<BannerView> CreateFileInfoView() const override
    {
        return DsRunnerFileInfoCardView::CreateShared(_colorScheme, _fontRepository);
    }

    SharedPtr<RecyclerViewBase> CreateCoverFlowRecyclerView() const override
    {
        return CarouselRecyclerView::CreateShared(_colorScheme);
    }

    SharedPtr<FileRecyclerAdapter> CreateCoverFlowRecyclerAdapter(
        RomBrowserViewModel* viewModel, const IThemeFileIconFactory* themeFileIconFactory,
        VBlankTextureLoader* vblankTextureLoader) const override
    {
        return SharedPtr<MaterialCoverFlowFileRecyclerAdapter>::MakeShared(
            viewModel->GetRomBrowserController(),
            &viewModel->GetFileInfoManager(),
            viewModel->GetIoTaskQueue(),
            themeFileIconFactory, this, vblankTextureLoader, &viewModel->GetCoverRepository());
    }

    Point GetTopCoverPosition() const override
    {
        return Point(75, 18);
    }

private:
    const MaterialColorScheme* _colorScheme;
    const IFontRepository* _fontRepository;
};
