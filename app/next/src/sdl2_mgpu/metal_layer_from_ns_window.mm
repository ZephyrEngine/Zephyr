
#import <Cocoa/Cocoa.h>
#import <QuartzCore/CAMetalLayer.h>

extern "C" CAMetalLayer* metal_layer_from_ns_window(NSWindow* ns_window) {
  CAMetalLayer* metal_layer = nullptr;
  [ns_window.contentView setWantsLayer : YES];
  metal_layer = [CAMetalLayer layer];
  [ns_window.contentView setLayer : metal_layer];
  return metal_layer;
}