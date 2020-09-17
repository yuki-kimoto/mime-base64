on 'runtime' => sub {
    requires 'strict';
    requires 'vars';
    requires 'Exporter';
    requires 'XSLoader';
};

on 'configure' => sub {
    requires 'ExtUtils::MakeMaker';
};

on 'test' => sub {
    requires 'strict';
    requires 'Test';
};
