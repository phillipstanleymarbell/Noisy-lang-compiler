dimensionTypeNames {
    time = "s";
}

law {
    velocity = distance / time;
    acceleration = velocity / time;
    force = mass * acceleration;
    work = dot(force, displacement);
}

dimensionAliases {
    work = "J";
}

vectorIntegrals {
    [displacement, velocity, acceleration];
}

scalarIntegrals {
    [distance, speed, scalar_acceleration];
}
