#import <Foundation/Foundation.h>
#import <ApplicationServices/ApplicationServices.h>
int main(){@autoreleasepool{NSDictionary* result=@{@"accessibility_trusted":@(AXIsProcessTrusted()),@"permissions_requested":@NO};
    NSData* data=[NSJSONSerialization dataWithJSONObject:result options:NSJSONWritingPrettyPrinted error:nil];fwrite(data.bytes,1,data.length,stdout);fputc('\n',stdout);return 0;}}
