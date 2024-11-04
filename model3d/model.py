import cadquery as cq

# The dimensions of the LED module
## Box
led_length = 5.0
led_width = 5.0
led_height = 1.57
## Inward cone
led_hole_dia1 = 4.0
led_hole_dia2 = 3.35
led_hole_depth = 0.8

# Create a single LED based on the dimensions
led_single = (
    cq.Workplane("XY")
    .rect(led_length, led_width)
    .extrude(led_height)
    .faces(">Z")
    .circle(led_hole_dia1/2)
    .workplane(offset=-led_hole_depth)
    .circle(led_hole_dia2/2)
    .loft(combine="cut"))


result = led_single

show_object(result)