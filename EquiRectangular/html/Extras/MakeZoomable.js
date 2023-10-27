// Copyright (C) 2023 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

function enableZoom() {
    var zoomables, i, el;
    
    zoomables = document.getElementsByClassName("zoomable");
    
    for (i = 0; i < zoomables.length; i++) {
        el = zoomables[i];
        el.style.cursor = "zoom-in"
        el.onmousedown = toggleZoom;
    }
}

function toggleZoom(e) {
    if (e.target.style.cursor == "zoom-in")
    {
        e.target.style.cursor = "zoom-out"
        e.target.style.maxWidth = ""
    }
    else if (e.target.style.cursor == "zoom-out")
    {
        e.target.style.cursor = "zoom-in"
        e.target.style.maxWidth = "100%"
    }
}
