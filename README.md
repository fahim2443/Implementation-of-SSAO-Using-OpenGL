# Implementation-of-SSAO-Using-OpenGL

## Project Summary
Screen Space Ambient Occlusion (SSAO) is implemented with a four-pass deferred shading pipeline: geometry pass to fill the G-Buffer (position, normal, albedo), SSAO pass to compute occlusion using a 64-sample hemisphere kernel and tiled noise texture, blur pass to denoise the occlusion buffer, and lighting pass to combine ambient, diffuse, and specular terms with the blurred AO. The demo loads the Stanford Dragon, renders a floor, supports a first-person camera (WASD + mouse + scroll), and achieves real-time performance at 1280x720.

## Report Outline (7–9 pages)
- Task and Objectives: purpose, goals, performance target (>=60 FPS @ 1280x720)
- Theory and Algorithm: deferred shading, SSAO sampling (kernel, noise, bias, radius), blur filter, lighting integration
- Implementation: key code (G-Buffer setup, kernel/noise generation, SSAO shader, blur shader, lighting shader, main loop, camera)
- Results: environment (macOS, OpenGL 3.3), visuals (with vs without SSAO), performance summary
- Issues and Fixes: texture format precision (RGBA16F), sample distribution (quadratic scale), acne mitigation (bias + range check)
- Conclusion: achieved objectives, limitations of screen-space AO, future improvements (temporal/bilateral filtering)

## Submission Formalities
- Include screenshots of key code sections (G-Buffer setup, SSAO kernel/noise, SSAO shader, blur shader, lighting shader, main loop, camera) and rendered outputs (with SSAO, without SSAO comparison).
- Use long-form academic prose (no bullets in the report body), objective tone, and figure citations (e.g., "as shown in Figure 3.2").
- Keep figures reasonably sized and placed near references; avoid large blank areas.
- Follow required sections and length (7–9 pages), font 12pt, line spacing 1.5–2.0, 1-inch margins, page numbers.
- Cite sources (LearnOpenGL SSAO, OpenGL Programming Guide, Real-Time Rendering) in a numbered reference list.