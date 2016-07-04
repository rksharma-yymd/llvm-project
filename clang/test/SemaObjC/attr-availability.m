// RUN: %clang_cc1 -triple x86_64-apple-darwin9.0.0 -fsyntax-only -verify %s
// RUN: %clang_cc1 -D WARN_PARTIAL -Wpartial-availability -triple x86_64-apple-darwin9.0.0 -fsyntax-only -verify %s

@protocol P
- (void)proto_method __attribute__((availability(macosx,introduced=10.1,deprecated=10.2))); // expected-note 2 {{'proto_method' has been explicitly marked deprecated here}}

#if defined(WARN_PARTIAL)
  // expected-note@+2 2 {{'partial_proto_method' has been explicitly marked partial here}}
#endif
- (void)partial_proto_method __attribute__((availability(macosx,introduced=10.8)));
@end

@interface A <P>
- (void)method __attribute__((availability(macosx,introduced=10.1,deprecated=10.2))); // expected-note {{'method' has been explicitly marked deprecated here}}
#if defined(WARN_PARTIAL)
  // expected-note@+2 {{'partialMethod' has been explicitly marked partial here}}
#endif
- (void)partialMethod __attribute__((availability(macosx,introduced=10.8)));

- (void)overridden __attribute__((availability(macosx,introduced=10.3))); // expected-note{{overridden method is here}}
- (void)overridden2 __attribute__((availability(macosx,introduced=10.3)));
- (void)overridden3 __attribute__((availability(macosx,deprecated=10.3)));
- (void)overridden4 __attribute__((availability(macosx,deprecated=10.3))); // expected-note{{overridden method is here}}
- (void)overridden5 __attribute__((availability(macosx,unavailable)));
- (void)overridden6 __attribute__((availability(macosx,introduced=10.3))); // expected-note{{overridden method is here}}
- (void)unavailableMethod __attribute__((unavailable));
@end

// rdar://11475360
@interface B : A
- (void)method; // NOTE: we expect 'method' to *not* inherit availability.
- (void)partialMethod; // Likewise.
- (void)overridden __attribute__((availability(macosx,introduced=10.4))); // expected-warning{{overriding method introduced after overridden method on macOS (10.4 vs. 10.3)}}
- (void)overridden2 __attribute__((availability(macosx,introduced=10.2)));
- (void)overridden3 __attribute__((availability(macosx,deprecated=10.4)));
- (void)overridden4 __attribute__((availability(macosx,deprecated=10.2))); // expected-warning{{overriding method deprecated before overridden method on macOS (10.3 vs. 10.2)}}
- (void)overridden5 __attribute__((availability(macosx,introduced=10.3)));
- (void)overridden6 __attribute__((availability(macosx,unavailable))); // expected-warning{{overriding method cannot be unavailable on macOS when its overridden method is available}}
- (void)unavailableMethod; // does *not* inherit unavailability
@end

void f(A *a, B *b) {
  [a method]; // expected-warning{{'method' is deprecated: first deprecated in macOS 10.2}}
  [b method]; // no-warning
  [a proto_method]; // expected-warning{{'proto_method' is deprecated: first deprecated in macOS 10.2}}
  [b proto_method]; // expected-warning{{'proto_method' is deprecated: first deprecated in macOS 10.2}}

#if defined(WARN_PARTIAL)
  // expected-warning@+2 {{'partialMethod' is partial: introduced in macOS 10.8}} expected-note@+2 {{explicitly redeclare 'partialMethod' to silence this warning}}
#endif
  [a partialMethod];
  [b partialMethod];  // no warning
#if defined(WARN_PARTIAL)
  // expected-warning@+2 {{'partial_proto_method' is partial: introduced in macOS 10.8}} expected-note@+2 {{explicitly redeclare 'partial_proto_method' to silence this warning}}
#endif
  [a partial_proto_method];
#if defined(WARN_PARTIAL)
  // expected-warning@+2 {{'partial_proto_method' is partial: introduced in macOS 10.8}} expected-note@+2 {{explicitly redeclare 'partial_proto_method' to silence this warning}}
#endif
  [b partial_proto_method];
}

@interface A (NewAPI)
- (void)partialMethod;
- (void)partial_proto_method;
@end

void f_after_redecl(A *a, B *b) {
  [a partialMethod]; // no warning
  [b partialMethod]; // no warning
  [a partial_proto_method]; // no warning
  [b partial_proto_method]; // no warning
}

// Test case for <rdar://problem/11627873>.  Warn about
// using a deprecated method when that method is re-implemented in a
// subclass where the redeclared method is not deprecated.
@interface C
- (void) method __attribute__((availability(macosx,introduced=10.1,deprecated=10.2))); // expected-note {{'method' has been explicitly marked deprecated here}}
@end

@interface D : C
- (void) method;
@end

@interface E : D
- (void) method;
@end

@implementation D
- (void) method {
  [super method]; // expected-warning {{'method' is deprecated: first deprecated in macOS 10.2}}
}
@end

@implementation E
- (void) method {
  [super method]; // no-warning
}
@end

// rdar://18059669
@class NSMutableArray;

@interface NSDictionary
+ (instancetype)dictionaryWithObjectsAndKeys:(id)firstObject, ... __attribute__((sentinel(0,1)));
@end

@class NSString;

extern NSString *NSNibTopLevelObjects __attribute__((availability(macosx,introduced=10.0 ,deprecated=10.8,message="" )));
id NSNibOwner, topNibObjects;

@interface AppDelegate (SIEImport) // expected-error {{cannot find interface declaration for 'AppDelegate'}}

-(void)__attribute__((ibaction))importFromSIE:(id)sender;

@end

@implementation AppDelegate (SIEImport) // expected-error {{cannot find interface declaration for 'AppDelegate'}}

-(void)__attribute__((ibaction))importFromSIE:(id)sender {

 NSMutableArray *topNibObjects;
 NSDictionary *nibLoadDict = [NSDictionary dictionaryWithObjectsAndKeys:self, NSNibOwner, topNibObjects, NSNibTopLevelObjects, ((void *)0)];
}

@end

@protocol PartialProt
- (void)ppartialMethod __attribute__((availability(macosx,introduced=10.8)));
+ (void)ppartialMethod __attribute__((availability(macosx,introduced=10.8)));
@end

@interface PartialI <PartialProt>
- (void)partialMethod __attribute__((availability(macosx,introduced=10.8)));
+ (void)partialMethod __attribute__((availability(macosx,introduced=10.8)));
@end

@interface PartialI ()
- (void)ipartialMethod1 __attribute__((availability(macosx,introduced=10.8)));
#if defined(WARN_PARTIAL)
  // expected-note@+2 {{'ipartialMethod2' has been explicitly marked partial here}}
#endif
- (void)ipartialMethod2 __attribute__((availability(macosx,introduced=10.8)));
+ (void)ipartialMethod1 __attribute__((availability(macosx,introduced=10.8)));
#if defined(WARN_PARTIAL)
  // expected-note@+2 {{'ipartialMethod2' has been explicitly marked partial here}}
#endif
+ (void)ipartialMethod2 __attribute__((availability(macosx,introduced=10.8)));
@end

@interface PartialI (Redecls)
- (void)partialMethod;
- (void)ipartialMethod1;
- (void)ppartialMethod;
+ (void)partialMethod;
+ (void)ipartialMethod1;
+ (void)ppartialMethod;
@end

void partialfun(PartialI* a) {
  [a partialMethod]; // no warning
  [a ipartialMethod1]; // no warning
#if defined(WARN_PARTIAL)
  // expected-warning@+2 {{'ipartialMethod2' is partial: introduced in macOS 10.8}} expected-note@+2 {{explicitly redeclare 'ipartialMethod2' to silence this warning}}
#endif
  [a ipartialMethod2];
  [a ppartialMethod]; // no warning
  [PartialI partialMethod]; // no warning
  [PartialI ipartialMethod1]; // no warning
#if defined(WARN_PARTIAL)
  // expected-warning@+2 {{'ipartialMethod2' is partial: introduced in macOS 10.8}} expected-note@+2 {{explicitly redeclare 'ipartialMethod2' to silence this warning}}
#endif
  [PartialI ipartialMethod2];
  [PartialI ppartialMethod]; // no warning
}

#if defined(WARN_PARTIAL)
  // expected-note@+2 {{'PartialI2' has been explicitly marked partial here}}
#endif
__attribute__((availability(macosx, introduced = 10.8))) @interface PartialI2
@end

#if defined(WARN_PARTIAL)
  // expected-warning@+2 {{'PartialI2' is partial: introduced in macOS 10.8}} expected-note@+2 {{explicitly redeclare 'PartialI2' to silence this warning}}
#endif
void partialinter1(PartialI2* p) {
}

@class PartialI2;

void partialinter2(PartialI2* p) { // no warning
}


// Test that both the use of the 'typedef' and the enum constant
// produces an error. rdar://problem/20903588
#define UNAVAILABLE __attribute__((unavailable("not available")))

typedef enum MyEnum : int MyEnum;
enum MyEnum : int { // expected-note {{'MyEnum' has been explicitly marked unavailable here}}
  MyEnum_Blah UNAVAILABLE, // expected-note {{'MyEnum_Blah' has been explicitly marked unavailable here}}
} UNAVAILABLE;

void use_myEnum() {
  // expected-error@+2 {{'MyEnum' is unavailable: not available}}
  // expected-error@+1 {{MyEnum_Blah' is unavailable: not available}}
  MyEnum e = MyEnum_Blah; 
}

// Test that the availability of (optional) protocol methods is not
// inherited be implementations of those protocol methods.
@protocol AvailabilityP2
@optional
-(void)methodA __attribute__((availability(macosx,introduced=10.1,deprecated=10.2))); // expected-note 4{{'methodA' has been explicitly marked deprecated here}} \
// expected-note 2{{protocol method is here}}
-(void)methodB __attribute__((unavailable)); // expected-note 4{{'methodB' has been explicitly marked unavailable here}}
-(void)methodC;
@end

void testAvailabilityP2(id<AvailabilityP2> obj) {
  [obj methodA]; // expected-warning{{'methodA' is deprecated: first deprecated in macOS 10.2}}
  [obj methodB]; // expected-error{{'methodB' is unavailable}}
}

@interface ImplementsAvailabilityP2a <AvailabilityP2>
-(void)methodA;
-(void)methodB;
@end

void testImplementsAvailabilityP2a(ImplementsAvailabilityP2a *obj) {
  [obj methodA]; // okay: availability not inherited
  [obj methodB]; // okay: unavailability not inherited
}

__attribute__((objc_root_class))
@interface ImplementsAvailabilityP2b <AvailabilityP2>
@end

@implementation ImplementsAvailabilityP2b
-(void)methodA {
  // Make sure we're not inheriting availability.
  id<AvailabilityP2> obj = self;
  [obj methodA]; // expected-warning{{'methodA' is deprecated: first deprecated in macOS 10.2}}
  [obj methodB]; // expected-error{{'methodB' is unavailable}}
}
-(void)methodB {
  // Make sure we're not inheriting unavailability.
  id<AvailabilityP2> obj = self;
  [obj methodA]; // expected-warning{{'methodA' is deprecated: first deprecated in macOS 10.2}}
  [obj methodB]; // expected-error{{'methodB' is unavailable}}
}

@end

void testImplementsAvailabilityP2b(ImplementsAvailabilityP2b *obj) {
  // still get warnings/errors because we see the protocol version.

  [obj methodA]; // expected-warning{{'methodA' is deprecated: first deprecated in macOS 10.2}}
  [obj methodB]; // expected-error{{'methodB' is unavailable}}
}

__attribute__((objc_root_class))
@interface ImplementsAvailabilityP2c <AvailabilityP2>
-(void)methodA __attribute__((availability(macosx,introduced=10.2))); // expected-warning{{method introduced after the protocol method it implements on macOS (10.2 vs. 10.1)}}
-(void)methodB __attribute__((unavailable));
@end

__attribute__((objc_root_class))
@interface ImplementsAvailabilityP2d <AvailabilityP2>
@end

@implementation ImplementsAvailabilityP2d
-(void)methodA __attribute__((availability(macosx,introduced=10.2))) // expected-warning{{method introduced after the protocol method it implements on macOS (10.2 vs. 10.1)}}
{
}
-(void)methodB __attribute__((unavailable)) {
}
@end

__attribute__((objc_root_class))
@interface InheritUnavailableSuper
-(void)method __attribute__((unavailable)); // expected-note{{'method' has been explicitly marked unavailable here}}
@end

@interface InheritUnavailableSub : InheritUnavailableSuper
-(void)method;
@end

@implementation InheritUnavailableSub
-(void)method {
  InheritUnavailableSuper *obj = self;
  [obj method]; // expected-error{{'method' is unavailable}}
}
@end
