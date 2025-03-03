취업 포트폴리오 겸 개인 연구 프로젝트

# Zero Run Encoder
- 입력 데이터의 각 2-bit 패턴의 조건부 확률에 따른 2-bit 패턴 치환(변환) + 0bit-run에는 지수-골룸 부호화, 1bit-run에는 단항 부호화를 적용한 인코더입니다.
- 결과 파일은 [시그니처 + 키 데이터 + (2-bit 치환 후+)인코딩된 데이터] 로 구성돼 있습니다.

## 조건부 확률 2-bit 패턴 변환기
- 입력된 데이터를 일정 크기(KB)의 청크로 나눕니다.
- 각 청크의 특정 2-bit 패턴 다음에 오는 2-bit 패턴들의 빈도 수를 셉니다.
```
  ex) [00 00 10 00 11 10 00 11] 의 경우, [00 > 00 : 1회, 00 > 01 : 0회, 00 > 10 : 1회, 00 > 11 : 2회]    
```
- 가장 높은 빈도의 2-bit 패턴을 각 2-bit 패턴별 치환 테이블에 xor한다.
```
  ex) 00 다음에 오는 가장 높은 빈도의 2-bit 패턴이 11인 경우, 00의 치환 테이블 : [00 > 00^11, 00 > 01^11, 00 > 10^11, 00 > 11^11]    
```
- 설정된 치환 테이블로 입력된 파일의 데이터를 변환한다. (입력 데이터의 첫 2-bit : 결과 파일의 키 데이터에 기록한 후, 치환 테이블과 관계없이 항상 00으로 변환한다.)
```
입력 데이터가 [00 00 10 00 11 10 00 11] 이고, 00의 치환 테이블이 [00 > 00^11, 00 > 01^11, 00 > 10^11, 00 > 11^11]로 설정 됐을 때, 
출력 데이터 : [00 11 01 00 00 10 00 00]
```

## 작성 예정
