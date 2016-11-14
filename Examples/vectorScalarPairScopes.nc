dimensionTypeNames {
    time = "s";
    distance = "m";
    displacement = "m";
    mass = "g";
    angle = "rad";
}

law {
    velocity = distance / time;
    acceleration = velocity / time;
    force = mass * acceleration;
    # work = dot(force, displacement);
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

vectorScalarPairs {
    displacement = distance;
    velocity = speed;
    acceleration = scalar_acceleration;
}
