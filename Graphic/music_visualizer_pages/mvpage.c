#include "mvpage.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

mv_page_t *MusicVisualizerPage = NULL;

// Khai báo subpages khác
extern mv_page_t WaveformPage;
extern mv_page_t CircularPage;
extern mv_page_t ArcReactorPage;
extern mv_page_t ParticleFountainPage;
extern mv_page_t PeakMeterPage;

// Định nghĩa duy nhất list_subpages
mv_page_t *list_subpages[MAX_SUBPAGES] = {
    (mv_page_t*)&BasicMusicVisualizerPage,
    &CircularPage,
    &WaveformPage,
    &ArcReactorPage,
    &ParticleFountainPage,
    &PeakMeterPage,
    NULL, NULL, NULL, NULL
};

mv_page_err_code SetSubpage(uint16_t index) {
    if (index >= MAX_SUBPAGES || list_subpages[index] == NULL) {
        printf("Invalid subpage index: %d\n", index);
        return MV_PAGE_RET_FAIL;
    }
    MusicVisualizerPage = list_subpages[index];
    if (!MusicVisualizerPage) return MV_PAGE_RET_FAIL;
    return MV_PAGE_RET_OK;
}
