#include <widget/baseservice.h>

#undef slots
#include <pybind11/embed.h>
#include <pybind11/stl.h>
#define slots Q_SLOTS

namespace py = pybind11;

void initialInterop() {
    static py::scoped_interpreter guard{};
    static py::gil_scoped_release release;
}

void finalizeInterop() {

}
