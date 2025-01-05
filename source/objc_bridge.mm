#include "objc_bridge.h"
#import <Cocoa/Cocoa.h>

bool trashItem(const std::filesystem::path& path){
	auto str = path.wstring();
	NSString* pathstr = [[NSString alloc] initWithBytes:str.data() length:str.size() * sizeof(wchar_t) encoding:NSUTF32LittleEndianStringEncoding];
	NSURL* purl = [NSURL URLWithString:[NSString stringWithFormat:@"file://%@",[pathstr stringByAddingPercentEncodingWithAllowedCharacters:NSCharacterSet.URLQueryAllowedCharacterSet]]];
	NSError* error;
	bool result = [[NSFileManager defaultManager] trashItemAtURL:purl resultingItemURL:nil error:&error];
//	if (!result){
//		NSLog(@"Trash failed with reason: %@", error);
//	}
	return result;
}

void RevealFile(const std::filesystem::path& p){
    const auto str = p.string();
    auto urlstr = [NSString stringWithUTF8String:str.c_str()];
    [[NSWorkspace sharedWorkspace] selectFile:urlstr inFileViewerRootedAtPath:@""];
}
