취업 포트폴리오 겸 개인 연구 프로젝트

# Zero Run Encoder
## 프로젝트 소개
- 본 프로젝트는 독자적으로 고안한 파일 압축 알고리즘 검증 및 구현을 목표로 합니다.
- 입력된 데이터를 2-bit 단위로 조건부 확률에 따라 새로운 2-bit 패턴으로 치환(변환)한 후, '연속된 0b0'(Zero-bit-run)를 지수-골룸 부호화, '연속된 0b1'(One-bit-run)를 단항 부호화하는 인코더를 구현합니다.
- [파일 시그니처 + 키 데이터 + (2-bit 치환 후+)부호화된 데이터] 로 구성된 결과 파일을 생성합니다.

## 주요 모듈(압축 방식)
### - 조건부 확률 2-bit 패턴 변환기
#### 기능
- 가능한 길고 많은 Zero-bit-run 을 만들어, 지수-골룸 부호화의 효율을 높인다.
#### 작동 순서
  1. 입력된 데이터를 일정 크기(KB)의 청크로 나눈다.
  2. 각 청크의 특정 2-bit 패턴 다음에 오는 2-bit 패턴들의 빈도 수를 센다.
  ```
  [00 00 10 00 11 10 00 11] 의 경우, [00 > 00 : 1회, 00 > 01 : 0회, 00 > 10 : 1회, 00 > 11 : 2회]    
  ```
  3. 가장 높은 빈도의 2-bit 패턴을 각 2-bit 패턴별 치환 테이블에 xor한다.
  ```
  00 다음에 오는 가장 높은 빈도의 2-bit 패턴이 11인 경우, 00의 치환 테이블 : [00 > 00 XOR 11, 00 > 01 XOR 11, 00 > 10 XOR 11, 00 > 11 XOR 11]    
  ```
  4. 설정된 치환 테이블로 입력된 파일의 데이터를 변환한다. (입력 데이터의 첫 2-bit : 결과 파일의 키 데이터에 기록한 후, 치환 테이블과 관계없이 항상 0b00 으로 변환한다.)
  ```
  입력 데이터가 [00 00 10 00 11 10 00 11] 이고, 00의 치환 테이블이 [00 > 11, 00 > 10, 00 > 01, 00 > 00]로 설정 됐을 때,
  출력 데이터 : [00 11 01 00 00 10 00 00]
  ```
#### 특징
- 미리 계산된 변환 테이블을 활용하기 떄문에 변환 속도가 빠릅니다.
<br><br>

### - Zero & One-bit-run 인코더
#### 기능
- Zero-bit-run을 지수-골룸 부호화/복호화 한다.
  ##### - 부호화
  - ['Zero-bit-run 길이' < 3] 인 경우, ['Zero-bit-run 길이' + 1] 의 결과 값의 이진수 값으로 부호화한다.
  - ['Zero-bit-run 길이' >= 3] 인 경우,
    1. [log2('Zero-bit-run 길이' - 1)] 의 정수 값을 구한다.
    2. ['Zero-bit-run 길이' - 1] 의 결과 값의 이진수 값을 구한다.
    3. (1)에서 구한 정수 값 만큼 '연속된 0b0'를 추가하고 (2)의 이진수 값을 덧붙인다.
    ```
    부호화 : 입력 데이터가 [00 00 00 00 00 00 00 01] 인 경우, 'Zero-bit-run 길이' = 15    
      a. log2(15 - 1) ≒ 3.807 -> int(3.807) ∴ 0b000
      b. [15 - 1] = 14 ∴ 0b1110
      c. 15bits의 0b0 -> 0b0001110 으로 부호화 된다.
    ```
  ##### - 복호화
  - ['연속된 0b0 수' == 0] 인 경우, 2bits 만큼 읽어들여 해당 값만큼 Zero-bit-run 을 복호화 한다.
  - ['연속된 0b0 수' > 0] 인 경우,
    1. '연속된 0b0의 수'를 센다.
    2. ['연속된 0b0 수' + 1] 만큼 비트를 읽어 들인다.
    3. ['읽어 들인 비트의 값' + 1] 만큼 Zero-bit-run 을 복호화한다.
    ```
    복호화 : 부호화된 데이터가 [00 01 110] 인 경우,       
      a. '연속된 0b0 수' = 3 
      b. [3 + 1] = 4 ∴ 0b1110 을 읽어 들인다.
      c. [0b1110 + 1] = 15 ∴ 0b0001 110 -> 15bits의 0b0 으로 복호화 된다.
    ```
    <br>
    
- One-bit-run을 단항 부호화/복호화 한다.
  ##### - 부호화
    1. ['One-bit-run 길이' - 1] 만큼의 '연속된 0b0'를 추가한다.
    2. (1)의 '연속된 0b0' 끝에 0b1 을 덧붙인다.
    ```
    부호화 : 입력 데이터가 [11 11 11 11 11 11 11 10] 인 경우, 'One-bit-run 길이' = 15
      i. [15 - 1] = 14 -> 0b0000 0000 0000 00
     ii. 15bits의 0b1 -> 0b0000 0000 0000 001 으로 부호화 된다.
    ```
  ##### - 복호화
    1. '연속된 0b0의 수'를 센다.
    2. ['연속된 0b0의 수' + 1] 만큼 One-bit-run 을 복호화 한다.
    ```
    복호화 : 부호화된 데이터가 [00 00 00 00 00 00 00 1] 인 경우,
      i. '연속된 0b0의 수' = 14
     ii. [14 + 1] = 15 ∴ 0b0000 0000 0000 001 -> 15bits의 0b1 으로 복호화 된다.
    ```
#### 특징
- 부호화 : 미리 계산된 부호화 테이블을 활용하여 복호화에 비해 소요 시간이 짧습니다.
- 복호화 : 미리 계산된 복호화 테이블이 없습니다. 파일 입출력을 2개의 쓰레드로 나눠 처리함으로써 소요 시간을 약 20% 단축 시켰지만 여전히 부호화에 비해 오래 걸립니다.
<br><br><br>

## 컴파일 및 실행 방법
### - 환경
- OS : Windows 10 x64
- IDE : Dev-C++ 5.11
- Compiler : GCC 4.9.2 64-bit
### - 요구 사항
- Linker Option : -Wl,--stack,67108864
- Compiler Option : -pthread
- Include File DirPath : ZeroRunEncoder.0re/include
### - 실행 방법
- Encoding Mode :
```
ZeroRunEncoder.exe "encoding" "(InputFilePath)" "(OutputFilePath)"
```
- Decoding Mode :
```
ZeroRunEncoder.exe "decoding" "(0reFilePath)" "(OutputDirPath)"
```
- Show Details Mode :
```
ZeroRunEncoder.exe "details" "(0reFilePath)"
```
<br><br>

## 성능
- .
<br><br>

## 라이선스
- 이 프로젝트는 Apache License 2.0 하에 배포됩니다. 자세한 내용은 [LICENSE](LICENSE) 파일을 참조하세요.
<br><br>

## 노트
- GUI 프로그램에 연결 시켜 사용할 수 있습니다. (URL : https://github.com/sch1991/zero-run-encoder-gui)
- 리눅스 환경에서의 컴파일을 고려하여 개발하였으나, 직접 테스트 해보진 않았습니다.
