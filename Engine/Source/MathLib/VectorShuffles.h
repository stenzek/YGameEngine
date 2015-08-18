#pragma once

#define VECTOR2_SHUFFLE_FUNCTIONS(retType2)                     retType2 xx() const { return Shuffle2<0, 0>(); } \
                                                                retType2 xy() const { return Shuffle2<0, 1>(); } \
                                                                retType2 yx() const { return Shuffle2<1, 0>(); } \
                                                                retType2 yy() const { return Shuffle2<1, 1>(); }

#define VECTOR3_SHUFFLE_FUNCTIONS(retType2, retType3)           VECTOR2_SHUFFLE_FUNCTIONS(retType2) \
                                                                retType2 xz() const { return Shuffle2<0, 2>(); } \
                                                                retType2 yz() const { return Shuffle2<1, 2>(); } \
                                                                retType2 zx() const { return Shuffle2<2, 0>(); } \
                                                                retType2 zy() const { return Shuffle2<2, 1>(); } \
                                                                retType2 zz() const { return Shuffle2<2, 2>(); } \
                                                                retType3 xxx() const { return Shuffle3<0, 0, 0>(); } \
                                                                retType3 xxy() const { return Shuffle3<0, 0, 1>(); } \
                                                                retType3 xxz() const { return Shuffle3<0, 0, 2>(); } \
                                                                retType3 xyx() const { return Shuffle3<0, 1, 0>(); } \
                                                                retType3 xyy() const { return Shuffle3<0, 1, 1>(); } \
                                                                retType3 xyz() const { return Shuffle3<0, 1, 2>(); } \
                                                                retType3 xzx() const { return Shuffle3<0, 2, 0>(); } \
                                                                retType3 xzy() const { return Shuffle3<0, 2, 1>(); } \
                                                                retType3 xzz() const { return Shuffle3<0, 2, 2>(); } \
                                                                retType3 yxx() const { return Shuffle3<1, 0, 0>(); } \
                                                                retType3 yxy() const { return Shuffle3<1, 0, 1>(); } \
                                                                retType3 yxz() const { return Shuffle3<1, 0, 2>(); } \
                                                                retType3 yyx() const { return Shuffle3<1, 1, 0>(); } \
                                                                retType3 yyy() const { return Shuffle3<1, 1, 1>(); } \
                                                                retType3 yyz() const { return Shuffle3<1, 1, 2>(); } \
                                                                retType3 yzx() const { return Shuffle3<1, 2, 0>(); } \
                                                                retType3 yzy() const { return Shuffle3<1, 2, 1>(); } \
                                                                retType3 yzz() const { return Shuffle3<1, 2, 2>(); } \
                                                                retType3 zxx() const { return Shuffle3<2, 0, 0>(); } \
                                                                retType3 zxy() const { return Shuffle3<2, 0, 1>(); } \
                                                                retType3 zxz() const { return Shuffle3<2, 0, 2>(); } \
                                                                retType3 zyx() const { return Shuffle3<2, 1, 0>(); } \
                                                                retType3 zyy() const { return Shuffle3<2, 1, 1>(); } \
                                                                retType3 zyz() const { return Shuffle3<2, 1, 2>(); } \
                                                                retType3 zzx() const { return Shuffle3<2, 2, 0>(); } \
                                                                retType3 zzy() const { return Shuffle3<2, 2, 1>(); } \
                                                                retType3 zzz() const { return Shuffle3<2, 2, 2>(); }

#define VECTOR4_SHUFFLE_FUNCTIONS(retType2, retType3, retType4) VECTOR3_SHUFFLE_FUNCTIONS(retType2, retType3) \
                                                                retType2 xw() const { return Shuffle2<0, 3>(); } \
                                                                retType2 yw() const { return Shuffle2<1, 3>(); } \
                                                                retType2 zw() const { return Shuffle2<2, 3>(); } \
                                                                retType2 wx() const { return Shuffle2<3, 0>(); } \
                                                                retType2 wy() const { return Shuffle2<3, 1>(); } \
                                                                retType2 wz() const { return Shuffle2<3, 2>(); } \
                                                                retType2 ww() const { return Shuffle2<3, 2>(); } \
                                                                retType3 xxw() const { return Shuffle3<0, 0, 3>(); } \
                                                                retType3 xyw() const { return Shuffle3<0, 1, 3>(); } \
                                                                retType3 xzw() const { return Shuffle3<0, 2, 3>(); } \
                                                                retType3 xwx() const { return Shuffle3<0, 3, 0>(); } \
                                                                retType3 xwy() const { return Shuffle3<0, 3, 1>(); } \
                                                                retType3 xwz() const { return Shuffle3<0, 3, 2>(); } \
                                                                retType3 xww() const { return Shuffle3<0, 3, 3>(); } \
                                                                retType3 yxw() const { return Shuffle3<1, 0, 3>(); } \
                                                                retType3 yyw() const { return Shuffle3<1, 1, 3>(); } \
                                                                retType3 yzw() const { return Shuffle3<1, 2, 3>(); } \
                                                                retType3 ywx() const { return Shuffle3<1, 3, 0>(); } \
                                                                retType3 ywy() const { return Shuffle3<1, 3, 1>(); } \
                                                                retType3 ywz() const { return Shuffle3<1, 3, 2>(); } \
                                                                retType3 yww() const { return Shuffle3<1, 3, 3>(); } \
                                                                retType3 zxw() const { return Shuffle3<2, 0, 3>(); } \
                                                                retType3 zyw() const { return Shuffle3<2, 1, 3>(); } \
                                                                retType3 zzw() const { return Shuffle3<2, 2, 3>(); } \
                                                                retType3 zwx() const { return Shuffle3<2, 3, 0>(); } \
                                                                retType3 zwy() const { return Shuffle3<2, 3, 1>(); } \
                                                                retType3 zwz() const { return Shuffle3<2, 3, 2>(); } \
                                                                retType3 zww() const { return Shuffle3<2, 3, 3>(); } \
                                                                retType3 wxw() const { return Shuffle3<3, 0, 3>(); } \
                                                                retType3 wyw() const { return Shuffle3<3, 1, 3>(); } \
                                                                retType3 wzw() const { return Shuffle3<3, 2, 3>(); } \
                                                                retType3 wwx() const { return Shuffle3<3, 3, 0>(); } \
                                                                retType3 wwy() const { return Shuffle3<3, 3, 1>(); } \
                                                                retType3 wwz() const { return Shuffle3<3, 3, 2>(); } \
                                                                retType3 www() const { return Shuffle3<3, 3, 3>(); } \
                                                                retType4 xxxx() const { return Shuffle4<0, 0, 0, 0>(); } \
                                                                retType4 xxxy() const { return Shuffle4<0, 1, 0, 1>(); } \
                                                                retType4 xxxz() const { return Shuffle4<0, 1, 0, 2>(); } \
                                                                retType4 xxxw() const { return Shuffle4<0, 1, 0, 3>(); } \
                                                                retType4 xxyx() const { return Shuffle4<0, 1, 1, 0>(); } \
                                                                retType4 xxyy() const { return Shuffle4<0, 1, 1, 1>(); } \
                                                                retType4 xxyz() const { return Shuffle4<0, 1, 1, 2>(); } \
                                                                retType4 xxyw() const { return Shuffle4<0, 1, 1, 3>(); } \
                                                                retType4 xxzx() const { return Shuffle4<0, 1, 2, 0>(); } \
                                                                retType4 xxzy() const { return Shuffle4<0, 1, 2, 1>(); } \
                                                                retType4 xxzz() const { return Shuffle4<0, 1, 2, 2>(); } \
                                                                retType4 xxzw() const { return Shuffle4<0, 1, 2, 3>(); } \
                                                                retType4 xxwx() const { return Shuffle4<0, 1, 3, 0>(); } \
                                                                retType4 xxwy() const { return Shuffle4<0, 1, 3, 1>(); } \
                                                                retType4 xxwz() const { return Shuffle4<0, 1, 3, 2>(); } \
                                                                retType4 xxww() const { return Shuffle4<0, 1, 3, 3>(); } \
                                                                retType4 xyxx() const { return Shuffle4<0, 1, 0, 0>(); } \
                                                                retType4 xyxy() const { return Shuffle4<0, 1, 0, 1>(); } \
                                                                retType4 xyxz() const { return Shuffle4<0, 1, 0, 2>(); } \
                                                                retType4 xyxw() const { return Shuffle4<0, 1, 0, 3>(); } \
                                                                retType4 xyyx() const { return Shuffle4<0, 1, 1, 0>(); } \
                                                                retType4 xyyy() const { return Shuffle4<0, 1, 1, 1>(); } \
                                                                retType4 xyyz() const { return Shuffle4<0, 1, 1, 2>(); } \
                                                                retType4 xyyw() const { return Shuffle4<0, 1, 1, 3>(); } \
                                                                retType4 xyzx() const { return Shuffle4<0, 1, 2, 0>(); } \
                                                                retType4 xyzy() const { return Shuffle4<0, 1, 2, 1>(); } \
                                                                retType4 xyzz() const { return Shuffle4<0, 1, 2, 2>(); } \
                                                                retType4 xyzw() const { return Shuffle4<0, 1, 2, 3>(); } \
                                                                retType4 xywx() const { return Shuffle4<0, 1, 3, 0>(); } \
                                                                retType4 xywy() const { return Shuffle4<0, 1, 3, 1>(); } \
                                                                retType4 xywz() const { return Shuffle4<0, 1, 3, 2>(); } \
                                                                retType4 xyww() const { return Shuffle4<0, 1, 3, 3>(); } \
                                                                retType4 xzxx() const { return Shuffle4<0, 2, 0, 0>(); } \
                                                                retType4 xzxy() const { return Shuffle4<0, 2, 0, 1>(); } \
                                                                retType4 xzxz() const { return Shuffle4<0, 2, 0, 2>(); } \
                                                                retType4 xzxw() const { return Shuffle4<0, 2, 0, 3>(); } \
                                                                retType4 xzyx() const { return Shuffle4<0, 2, 1, 0>(); } \
                                                                retType4 xzyy() const { return Shuffle4<0, 2, 1, 1>(); } \
                                                                retType4 xzyz() const { return Shuffle4<0, 2, 1, 2>(); } \
                                                                retType4 xzyw() const { return Shuffle4<0, 2, 1, 3>(); } \
                                                                retType4 xzzx() const { return Shuffle4<0, 2, 2, 0>(); } \
                                                                retType4 xzzy() const { return Shuffle4<0, 2, 2, 1>(); } \
                                                                retType4 xzzz() const { return Shuffle4<0, 2, 2, 2>(); } \
                                                                retType4 xzzw() const { return Shuffle4<0, 2, 2, 3>(); } \
                                                                retType4 xzwx() const { return Shuffle4<0, 2, 3, 0>(); } \
                                                                retType4 xzwy() const { return Shuffle4<0, 2, 3, 1>(); } \
                                                                retType4 xzwz() const { return Shuffle4<0, 2, 3, 2>(); } \
                                                                retType4 xzww() const { return Shuffle4<0, 2, 3, 3>(); } \
                                                                retType4 xwxx() const { return Shuffle4<0, 3, 0, 0>(); } \
                                                                retType4 xwxy() const { return Shuffle4<0, 3, 0, 1>(); } \
                                                                retType4 xwxz() const { return Shuffle4<0, 3, 0, 2>(); } \
                                                                retType4 xwxw() const { return Shuffle4<0, 3, 0, 3>(); } \
                                                                retType4 xwyx() const { return Shuffle4<0, 3, 1, 0>(); } \
                                                                retType4 xwyy() const { return Shuffle4<0, 3, 1, 1>(); } \
                                                                retType4 xwyz() const { return Shuffle4<0, 3, 1, 2>(); } \
                                                                retType4 xwyw() const { return Shuffle4<0, 3, 1, 3>(); } \
                                                                retType4 xwzx() const { return Shuffle4<0, 3, 2, 0>(); } \
                                                                retType4 xwzy() const { return Shuffle4<0, 3, 2, 1>(); } \
                                                                retType4 xwzz() const { return Shuffle4<0, 3, 2, 2>(); } \
                                                                retType4 xwzw() const { return Shuffle4<0, 3, 2, 3>(); } \
                                                                retType4 xwwx() const { return Shuffle4<0, 3, 3, 0>(); } \
                                                                retType4 xwwy() const { return Shuffle4<0, 3, 3, 1>(); } \
                                                                retType4 xwwz() const { return Shuffle4<0, 3, 3, 2>(); } \
                                                                retType4 xwww() const { return Shuffle4<0, 3, 3, 3>(); } \
                                                                retType4 yxxx() const { return Shuffle4<1, 0, 0, 0>(); } \
                                                                retType4 yxxy() const { return Shuffle4<1, 1, 0, 1>(); } \
                                                                retType4 yxxz() const { return Shuffle4<1, 1, 0, 2>(); } \
                                                                retType4 yxxw() const { return Shuffle4<1, 1, 0, 3>(); } \
                                                                retType4 yxyx() const { return Shuffle4<1, 1, 1, 0>(); } \
                                                                retType4 yxyy() const { return Shuffle4<1, 1, 1, 1>(); } \
                                                                retType4 yxyz() const { return Shuffle4<1, 1, 1, 2>(); } \
                                                                retType4 yxyw() const { return Shuffle4<1, 1, 1, 3>(); } \
                                                                retType4 yxzx() const { return Shuffle4<1, 1, 2, 0>(); } \
                                                                retType4 yxzy() const { return Shuffle4<1, 1, 2, 1>(); } \
                                                                retType4 yxzz() const { return Shuffle4<1, 1, 2, 2>(); } \
                                                                retType4 yxzw() const { return Shuffle4<1, 1, 2, 3>(); } \
                                                                retType4 yxwx() const { return Shuffle4<1, 1, 3, 0>(); } \
                                                                retType4 yxwy() const { return Shuffle4<1, 1, 3, 1>(); } \
                                                                retType4 yxwz() const { return Shuffle4<1, 1, 3, 2>(); } \
                                                                retType4 yxww() const { return Shuffle4<1, 1, 3, 3>(); } \
                                                                retType4 yyxx() const { return Shuffle4<1, 1, 0, 0>(); } \
                                                                retType4 yyxy() const { return Shuffle4<1, 1, 0, 1>(); } \
                                                                retType4 yyxz() const { return Shuffle4<1, 1, 0, 2>(); } \
                                                                retType4 yyxw() const { return Shuffle4<1, 1, 0, 3>(); } \
                                                                retType4 yyyx() const { return Shuffle4<1, 1, 1, 0>(); } \
                                                                retType4 yyyy() const { return Shuffle4<1, 1, 1, 1>(); } \
                                                                retType4 yyyz() const { return Shuffle4<1, 1, 1, 2>(); } \
                                                                retType4 yyyw() const { return Shuffle4<1, 1, 1, 3>(); } \
                                                                retType4 yyzx() const { return Shuffle4<1, 1, 2, 0>(); } \
                                                                retType4 yyzy() const { return Shuffle4<1, 1, 2, 1>(); } \
                                                                retType4 yyzz() const { return Shuffle4<1, 1, 2, 2>(); } \
                                                                retType4 yyzw() const { return Shuffle4<1, 1, 2, 3>(); } \
                                                                retType4 yywx() const { return Shuffle4<1, 1, 3, 0>(); } \
                                                                retType4 yywy() const { return Shuffle4<1, 1, 3, 1>(); } \
                                                                retType4 yywz() const { return Shuffle4<1, 1, 3, 2>(); } \
                                                                retType4 yyww() const { return Shuffle4<1, 1, 3, 3>(); } \
                                                                retType4 yzxx() const { return Shuffle4<1, 2, 0, 0>(); } \
                                                                retType4 yzxy() const { return Shuffle4<1, 2, 0, 1>(); } \
                                                                retType4 yzxz() const { return Shuffle4<1, 2, 0, 2>(); } \
                                                                retType4 yzxw() const { return Shuffle4<1, 2, 0, 3>(); } \
                                                                retType4 yzyx() const { return Shuffle4<1, 2, 1, 0>(); } \
                                                                retType4 yzyy() const { return Shuffle4<1, 2, 1, 1>(); } \
                                                                retType4 yzyz() const { return Shuffle4<1, 2, 1, 2>(); } \
                                                                retType4 yzyw() const { return Shuffle4<1, 2, 1, 3>(); } \
                                                                retType4 yzzx() const { return Shuffle4<1, 2, 2, 0>(); } \
                                                                retType4 yzzy() const { return Shuffle4<1, 2, 2, 1>(); } \
                                                                retType4 yzzz() const { return Shuffle4<1, 2, 2, 2>(); } \
                                                                retType4 yzzw() const { return Shuffle4<1, 2, 2, 3>(); } \
                                                                retType4 yzwx() const { return Shuffle4<1, 2, 3, 0>(); } \
                                                                retType4 yzwy() const { return Shuffle4<1, 2, 3, 1>(); } \
                                                                retType4 yzwz() const { return Shuffle4<1, 2, 3, 2>(); } \
                                                                retType4 yzww() const { return Shuffle4<1, 2, 3, 3>(); } \
                                                                retType4 ywxx() const { return Shuffle4<1, 3, 0, 0>(); } \
                                                                retType4 ywxy() const { return Shuffle4<1, 3, 0, 1>(); } \
                                                                retType4 ywxz() const { return Shuffle4<1, 3, 0, 2>(); } \
                                                                retType4 ywxw() const { return Shuffle4<1, 3, 0, 3>(); } \
                                                                retType4 ywyx() const { return Shuffle4<1, 3, 1, 0>(); } \
                                                                retType4 ywyy() const { return Shuffle4<1, 3, 1, 1>(); } \
                                                                retType4 ywyz() const { return Shuffle4<1, 3, 1, 2>(); } \
                                                                retType4 ywyw() const { return Shuffle4<1, 3, 1, 3>(); } \
                                                                retType4 ywzx() const { return Shuffle4<1, 3, 2, 0>(); } \
                                                                retType4 ywzy() const { return Shuffle4<1, 3, 2, 1>(); } \
                                                                retType4 ywzz() const { return Shuffle4<1, 3, 2, 2>(); } \
                                                                retType4 ywzw() const { return Shuffle4<1, 3, 2, 3>(); } \
                                                                retType4 ywwx() const { return Shuffle4<1, 3, 3, 0>(); } \
                                                                retType4 ywwy() const { return Shuffle4<1, 3, 3, 1>(); } \
                                                                retType4 ywwz() const { return Shuffle4<1, 3, 3, 2>(); } \
                                                                retType4 ywww() const { return Shuffle4<1, 3, 3, 3>(); } \
                                                                retType4 zxxx() const { return Shuffle4<2, 0, 0, 0>(); } \
                                                                retType4 zxxy() const { return Shuffle4<2, 1, 0, 1>(); } \
                                                                retType4 zxxz() const { return Shuffle4<2, 1, 0, 2>(); } \
                                                                retType4 zxxw() const { return Shuffle4<2, 1, 0, 3>(); } \
                                                                retType4 zxyx() const { return Shuffle4<2, 1, 1, 0>(); } \
                                                                retType4 zxyy() const { return Shuffle4<2, 1, 1, 1>(); } \
                                                                retType4 zxyz() const { return Shuffle4<2, 1, 1, 2>(); } \
                                                                retType4 zxyw() const { return Shuffle4<2, 1, 1, 3>(); } \
                                                                retType4 zxzx() const { return Shuffle4<2, 1, 2, 0>(); } \
                                                                retType4 zxzy() const { return Shuffle4<2, 1, 2, 1>(); } \
                                                                retType4 zxzz() const { return Shuffle4<2, 1, 2, 2>(); } \
                                                                retType4 zxzw() const { return Shuffle4<2, 1, 2, 3>(); } \
                                                                retType4 zxwx() const { return Shuffle4<2, 1, 3, 0>(); } \
                                                                retType4 zxwy() const { return Shuffle4<2, 1, 3, 1>(); } \
                                                                retType4 zxwz() const { return Shuffle4<2, 1, 3, 2>(); } \
                                                                retType4 zxww() const { return Shuffle4<2, 1, 3, 3>(); } \
                                                                retType4 zyxx() const { return Shuffle4<2, 1, 0, 0>(); } \
                                                                retType4 zyxy() const { return Shuffle4<2, 1, 0, 1>(); } \
                                                                retType4 zyxz() const { return Shuffle4<2, 1, 0, 2>(); } \
                                                                retType4 zyxw() const { return Shuffle4<2, 1, 0, 3>(); } \
                                                                retType4 zyyx() const { return Shuffle4<2, 1, 1, 0>(); } \
                                                                retType4 zyyy() const { return Shuffle4<2, 1, 1, 1>(); } \
                                                                retType4 zyyz() const { return Shuffle4<2, 1, 1, 2>(); } \
                                                                retType4 zyyw() const { return Shuffle4<2, 1, 1, 3>(); } \
                                                                retType4 zyzx() const { return Shuffle4<2, 1, 2, 0>(); } \
                                                                retType4 zyzy() const { return Shuffle4<2, 1, 2, 1>(); } \
                                                                retType4 zyzz() const { return Shuffle4<2, 1, 2, 2>(); } \
                                                                retType4 zyzw() const { return Shuffle4<2, 1, 2, 3>(); } \
                                                                retType4 zywx() const { return Shuffle4<2, 1, 3, 0>(); } \
                                                                retType4 zywy() const { return Shuffle4<2, 1, 3, 1>(); } \
                                                                retType4 zywz() const { return Shuffle4<2, 1, 3, 2>(); } \
                                                                retType4 zyww() const { return Shuffle4<2, 1, 3, 3>(); } \
                                                                retType4 zzxx() const { return Shuffle4<2, 2, 0, 0>(); } \
                                                                retType4 zzxy() const { return Shuffle4<2, 2, 0, 1>(); } \
                                                                retType4 zzxz() const { return Shuffle4<2, 2, 0, 2>(); } \
                                                                retType4 zzxw() const { return Shuffle4<2, 2, 0, 3>(); } \
                                                                retType4 zzyx() const { return Shuffle4<2, 2, 1, 0>(); } \
                                                                retType4 zzyy() const { return Shuffle4<2, 2, 1, 1>(); } \
                                                                retType4 zzyz() const { return Shuffle4<2, 2, 1, 2>(); } \
                                                                retType4 zzyw() const { return Shuffle4<2, 2, 1, 3>(); } \
                                                                retType4 zzzx() const { return Shuffle4<2, 2, 2, 0>(); } \
                                                                retType4 zzzy() const { return Shuffle4<2, 2, 2, 1>(); } \
                                                                retType4 zzzz() const { return Shuffle4<2, 2, 2, 2>(); } \
                                                                retType4 zzzw() const { return Shuffle4<2, 2, 2, 3>(); } \
                                                                retType4 zzwx() const { return Shuffle4<2, 2, 3, 0>(); } \
                                                                retType4 zzwy() const { return Shuffle4<2, 2, 3, 1>(); } \
                                                                retType4 zzwz() const { return Shuffle4<2, 2, 3, 2>(); } \
                                                                retType4 zzww() const { return Shuffle4<2, 2, 3, 3>(); } \
                                                                retType4 zwxx() const { return Shuffle4<2, 3, 0, 0>(); } \
                                                                retType4 zwxy() const { return Shuffle4<2, 3, 0, 1>(); } \
                                                                retType4 zwxz() const { return Shuffle4<2, 3, 0, 2>(); } \
                                                                retType4 zwxw() const { return Shuffle4<2, 3, 0, 3>(); } \
                                                                retType4 zwyx() const { return Shuffle4<2, 3, 1, 0>(); } \
                                                                retType4 zwyy() const { return Shuffle4<2, 3, 1, 1>(); } \
                                                                retType4 zwyz() const { return Shuffle4<2, 3, 1, 2>(); } \
                                                                retType4 zwyw() const { return Shuffle4<2, 3, 1, 3>(); } \
                                                                retType4 zwzx() const { return Shuffle4<2, 3, 2, 0>(); } \
                                                                retType4 zwzy() const { return Shuffle4<2, 3, 2, 1>(); } \
                                                                retType4 zwzz() const { return Shuffle4<2, 3, 2, 2>(); } \
                                                                retType4 zwzw() const { return Shuffle4<2, 3, 2, 3>(); } \
                                                                retType4 zwwx() const { return Shuffle4<2, 3, 3, 0>(); } \
                                                                retType4 zwwy() const { return Shuffle4<2, 3, 3, 1>(); } \
                                                                retType4 zwwz() const { return Shuffle4<2, 3, 3, 2>(); } \
                                                                retType4 zwww() const { return Shuffle4<2, 3, 3, 3>(); } \
                                                                retType4 wxxx() const { return Shuffle4<3, 0, 0, 0>(); } \
                                                                retType4 wxxy() const { return Shuffle4<3, 1, 0, 1>(); } \
                                                                retType4 wxxz() const { return Shuffle4<3, 1, 0, 2>(); } \
                                                                retType4 wxxw() const { return Shuffle4<3, 1, 0, 3>(); } \
                                                                retType4 wxyx() const { return Shuffle4<3, 1, 1, 0>(); } \
                                                                retType4 wxyy() const { return Shuffle4<3, 1, 1, 1>(); } \
                                                                retType4 wxyz() const { return Shuffle4<3, 1, 1, 2>(); } \
                                                                retType4 wxyw() const { return Shuffle4<3, 1, 1, 3>(); } \
                                                                retType4 wxzx() const { return Shuffle4<3, 1, 2, 0>(); } \
                                                                retType4 wxzy() const { return Shuffle4<3, 1, 2, 1>(); } \
                                                                retType4 wxzz() const { return Shuffle4<3, 1, 2, 2>(); } \
                                                                retType4 wxzw() const { return Shuffle4<3, 1, 2, 3>(); } \
                                                                retType4 wxwx() const { return Shuffle4<3, 1, 3, 0>(); } \
                                                                retType4 wxwy() const { return Shuffle4<3, 1, 3, 1>(); } \
                                                                retType4 wxwz() const { return Shuffle4<3, 1, 3, 2>(); } \
                                                                retType4 wxww() const { return Shuffle4<3, 1, 3, 3>(); } \
                                                                retType4 wyxx() const { return Shuffle4<3, 1, 0, 0>(); } \
                                                                retType4 wyxy() const { return Shuffle4<3, 1, 0, 1>(); } \
                                                                retType4 wyxz() const { return Shuffle4<3, 1, 0, 2>(); } \
                                                                retType4 wyxw() const { return Shuffle4<3, 1, 0, 3>(); } \
                                                                retType4 wyyx() const { return Shuffle4<3, 1, 1, 0>(); } \
                                                                retType4 wyyy() const { return Shuffle4<3, 1, 1, 1>(); } \
                                                                retType4 wyyz() const { return Shuffle4<3, 1, 1, 2>(); } \
                                                                retType4 wyyw() const { return Shuffle4<3, 1, 1, 3>(); } \
                                                                retType4 wyzx() const { return Shuffle4<3, 1, 2, 0>(); } \
                                                                retType4 wyzy() const { return Shuffle4<3, 1, 2, 1>(); } \
                                                                retType4 wyzz() const { return Shuffle4<3, 1, 2, 2>(); } \
                                                                retType4 wyzw() const { return Shuffle4<3, 1, 2, 3>(); } \
                                                                retType4 wywx() const { return Shuffle4<3, 1, 3, 0>(); } \
                                                                retType4 wywy() const { return Shuffle4<3, 1, 3, 1>(); } \
                                                                retType4 wywz() const { return Shuffle4<3, 1, 3, 2>(); } \
                                                                retType4 wyww() const { return Shuffle4<3, 1, 3, 3>(); } \
                                                                retType4 wzxx() const { return Shuffle4<3, 2, 0, 0>(); } \
                                                                retType4 wzxy() const { return Shuffle4<3, 2, 0, 1>(); } \
                                                                retType4 wzxz() const { return Shuffle4<3, 2, 0, 2>(); } \
                                                                retType4 wzxw() const { return Shuffle4<3, 2, 0, 3>(); } \
                                                                retType4 wzyx() const { return Shuffle4<3, 2, 1, 0>(); } \
                                                                retType4 wzyy() const { return Shuffle4<3, 2, 1, 1>(); } \
                                                                retType4 wzyz() const { return Shuffle4<3, 2, 1, 2>(); } \
                                                                retType4 wzyw() const { return Shuffle4<3, 2, 1, 3>(); } \
                                                                retType4 wzzx() const { return Shuffle4<3, 2, 2, 0>(); } \
                                                                retType4 wzzy() const { return Shuffle4<3, 2, 2, 1>(); } \
                                                                retType4 wzzz() const { return Shuffle4<3, 2, 2, 2>(); } \
                                                                retType4 wzzw() const { return Shuffle4<3, 2, 2, 3>(); } \
                                                                retType4 wzwx() const { return Shuffle4<3, 2, 3, 0>(); } \
                                                                retType4 wzwy() const { return Shuffle4<3, 2, 3, 1>(); } \
                                                                retType4 wzwz() const { return Shuffle4<3, 2, 3, 2>(); } \
                                                                retType4 wzww() const { return Shuffle4<3, 2, 3, 3>(); } \
                                                                retType4 wwxx() const { return Shuffle4<3, 3, 0, 0>(); } \
                                                                retType4 wwxy() const { return Shuffle4<3, 3, 0, 1>(); } \
                                                                retType4 wwxz() const { return Shuffle4<3, 3, 0, 2>(); } \
                                                                retType4 wwxw() const { return Shuffle4<3, 3, 0, 3>(); } \
                                                                retType4 wwyx() const { return Shuffle4<3, 3, 1, 0>(); } \
                                                                retType4 wwyy() const { return Shuffle4<3, 3, 1, 1>(); } \
                                                                retType4 wwyz() const { return Shuffle4<3, 3, 1, 2>(); } \
                                                                retType4 wwyw() const { return Shuffle4<3, 3, 1, 3>(); } \
                                                                retType4 wwzx() const { return Shuffle4<3, 3, 2, 0>(); } \
                                                                retType4 wwzy() const { return Shuffle4<3, 3, 2, 1>(); } \
                                                                retType4 wwzz() const { return Shuffle4<3, 3, 2, 2>(); } \
                                                                retType4 wwzw() const { return Shuffle4<3, 3, 2, 3>(); } \
                                                                retType4 wwwx() const { return Shuffle4<3, 3, 3, 0>(); } \
                                                                retType4 wwwy() const { return Shuffle4<3, 3, 3, 1>(); } \
                                                                retType4 wwwz() const { return Shuffle4<3, 3, 3, 2>(); } \
                                                                retType4 wwww() const { return Shuffle4<3, 3, 3, 3>(); }



