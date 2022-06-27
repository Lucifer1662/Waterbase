@0x957d93d69e1d06f6;

struct PersonRequest {
  name @0 :Text;
}

struct PersonResponse {
  name @0 :Text;
  email @1 :Text;
}


struct MessageType {
  type @0: Type;

  enum Type{
    personRequest @0;
    personResponse @1;
  }
}