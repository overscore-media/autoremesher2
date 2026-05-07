/*
 * C-callable hooks for OpenNL (C) to invoke AutoRemesher progress state held in C++ thread_locals.
 */
#include <geogram_report_progress.h>

extern "C" void geogram_nl_solver_begin(void)
{
    ++geogram_report_progress_round;
}

extern "C" void geogram_nl_solver_iter(unsigned int its, unsigned int max_iter)
{
    if (geogram_report_progress_callback && max_iter > 0)
        geogram_report_progress_callback(geogram_report_progress_tag,
            static_cast<float>(its) / static_cast<float>(max_iter));
}
